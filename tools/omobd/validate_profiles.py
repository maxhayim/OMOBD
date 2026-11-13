#!/usr/bin/env python3
"""validate_profiles.py -- check OMOBD engine profiles against the firmware.

Every name a profile uses (application, sensor, unit) is checked against the
registries compiled into the firmware (src/sensors/generated/registry_enums.h
and src/sensors/units_registry.cpp), so a profile can never point at a sensor
the firmware doesn't have. It also enforces OMOBD's safety rules:

  * A warning threshold may only carry a value when it is marked "validated"
    and names its source. Otherwise it must be null.
  * Config files ship with NO alarm rules unless the profile has validated
    thresholds.
  * A capability may only be "tested" if it names the test evidence.
  * Analog pin labels must match the ESP32-S3 mapping (A<n> = GPIO n+1).

Usage:
    python3 tools/omobd/validate_profiles.py          # exit 1 on any problem
"""
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PROFILES = os.path.join(REPO, "engine-profiles")

PROFILE_STATUSES = {"development", "planned", "supported"}
CAP_STATUSES = {"tested", "proposed", "unsupported", "unverified"}
THRESHOLD_STATUSES = {"unconfigured", "validated"}
ASPIRATIONS = {"naturally_aspirated", "turbocharged"}
DERIVED_KEYS = {"boost", "vacuum", "engineHours"}


def read_json(path):
    with open(path) as fh:
        return json.load(fh)


def read_text(path):
    with open(path) as fh:
        return fh.read()


def load_registry(repo=REPO):
    enums = read_text(os.path.join(repo, "src/sensors/generated/registry_enums.h"))
    sensors = set(re.findall(r"\bSENSOR_([A-Z0-9_]+)\s*=", enums)) - {"NONE"}
    apps = set(re.findall(r"\bAPP_([A-Z0-9_]+)\s*=", enums)) - {"NONE"}
    units_src = read_text(os.path.join(repo, "src/sensors/units_registry.cpp"))
    units = set(re.findall(r'PROGMEM = "([A-Za-z_/%]+)"', units_src))
    return {"sensors": sensors, "apps": apps, "units": {u.upper() for u in units}}


def esp32s3_analog_pin(label):
    m = re.fullmatch(r"A(\d+)", label or "")
    return int(m.group(1)) + 1 if m else None


def validate_config(path, reg, validated_thresholds, problems):
    try:
        cfg = read_json(path)
    except (OSError, ValueError) as e:
        problems.append(f"{path}: cannot read ({e})")
        return set()
    if cfg.get("schemaVersion") != 1 or cfg.get("mode") != "runtime":
        problems.append(f"{path}: schemaVersion must be 1 and mode 'runtime' (preOBD JSON IMPORT)")
    pins = set()
    for inp in cfg.get("inputs", []):
        where = f"{path}: input pin {inp.get('pin')}"
        if inp.get("app") not in reg["apps"]:
            problems.append(f"{where}: unknown application {inp.get('app')}")
        if inp.get("sensor") not in reg["sensors"]:
            problems.append(f"{where}: unknown sensor {inp.get('sensor')}")
        if str(inp.get("units", "")).upper() not in reg["units"]:
            problems.append(f"{where}: unknown units {inp.get('units')}")
        pin = inp.get("pin")
        if pin in pins:
            problems.append(f"{where}: pin used twice")
        pins.add(pin)
        expect = esp32s3_analog_pin(inp.get("pinLabel"))
        if inp.get("pinLabel") and expect != pin:
            problems.append(f"{where}: label {inp.get('pinLabel')} is GPIO {expect} on ESP32-S3")
        div = inp.get("divider")
        if div is not None and not (0 < div <= 1.0):
            problems.append(f"{where}: divider must be 0 < ratio <= 1")
    rules = cfg.get("rules")
    if rules is None:
        problems.append(f"{path}: must include an explicit 'rules' array (so no generic preset alarms are seeded)")
    elif rules and not validated_thresholds:
        problems.append(f"{path}: ships alarm rules but the profile has no validated thresholds")
    return pins


def validate_settings(path, cfg_pins, problems):
    try:
        s = read_json(path)
    except (OSError, ValueError) as e:
        problems.append(f"{path}: cannot read ({e})")
        return
    for k, v in (s.get("pins") or {}).items():
        if v is not None and v not in cfg_pins:
            problems.append(f"{path}: pins.{k} = {v} is not an input in the matching config")
    pts = (s.get("oilCurve") or {}).get("points") or []
    if pts:
        problems.append(f"{path}: oilCurve must ship empty; owners enter validated values")
    atm = (s.get("atmosphere") or {}).get("fixedBar")
    if atm is not None and not (0.5 <= atm <= 1.1):
        problems.append(f"{path}: atmosphere.fixedBar out of range")


def validate_profile(path, reg, problems):
    try:
        p = read_json(path)
    except (OSError, ValueError) as e:
        problems.append(f"{path}: cannot read ({e})")
        return None
    base = os.path.dirname(path)
    for key in ("profileSchema", "id", "name", "status", "identification", "capabilities", "thresholds"):
        if key not in p:
            problems.append(f"{path}: missing '{key}'")
    if p.get("status") not in PROFILE_STATUSES:
        problems.append(f"{path}: status must be one of {sorted(PROFILE_STATUSES)}")
    if os.path.basename(path) != f"{p.get('id')}.profile.json":
        problems.append(f"{path}: file name must be <id>.profile.json")
    ident = p.get("identification", {})
    if ident.get("verified") is True and not ident.get("source"):
        problems.append(f"{path}: identification marked verified without a source")

    apps_in_profile = set()
    for name, cap in (p.get("capabilities") or {}).items():
        where = f"{path}: capability {name}"
        st = cap.get("status")
        if st not in CAP_STATUSES:
            problems.append(f"{where}: status '{st}' not in {sorted(CAP_STATUSES)}")
        if st == "tested" and not cap.get("testedOn"):
            problems.append(f"{where}: 'tested' needs a 'testedOn' record (vehicle, date, what was compared)")
        if cap.get("app"):
            if cap["app"] not in reg["apps"]:
                problems.append(f"{where}: unknown application {cap['app']}")
            apps_in_profile.add(cap["app"])
        for s in cap.get("sensors", []):
            if s not in reg["sensors"]:
                problems.append(f"{where}: unknown sensor {s}")
        if cap.get("derived") and cap["derived"] not in DERIVED_KEYS:
            problems.append(f"{where}: unknown derived value {cap['derived']}")
        for a in cap.get("appliesTo", []):
            if a not in ASPIRATIONS:
                problems.append(f"{where}: unknown aspiration {a}")

    validated = False
    for name, t in (p.get("thresholds") or {}).items():
        if name.startswith("_"):
            continue
        where = f"{path}: threshold {name}"
        if t.get("status") not in THRESHOLD_STATUSES:
            problems.append(f"{where}: status must be one of {sorted(THRESHOLD_STATUSES)}")
        if t.get("value") is not None:
            if t.get("status") != "validated" or not t.get("source"):
                problems.append(f"{where}: has a value but is not 'validated' with a source")
            else:
                validated = True

    for rs in p.get("rpmSources", []):
        if rs.get("sensor") and rs["sensor"] not in reg["sensors"]:
            problems.append(f"{path}: rpm source {rs.get('id')}: unknown sensor {rs['sensor']}")

    dash = p.get("dashboard") or {}
    for page in dash.get("pages", []):
        for w in page.get("widgets", []):
            key = w.get("key", "")
            if key.startswith("derived."):
                if key[8:] not in DERIVED_KEYS:
                    problems.append(f"{path}: widget {key}: unknown derived value")
            elif key not in reg["apps"]:
                problems.append(f"{path}: widget {key}: unknown application")
            if w.get("type") not in ("dial", "bar", "number"):
                problems.append(f"{path}: widget {key}: type must be dial, bar or number")

    for v in p.get("variants", []):
        if v.get("aspiration") not in ASPIRATIONS:
            problems.append(f"{path}: variant {v.get('id')}: unknown aspiration")
        pins = set()
        if v.get("config"):
            cfg_path = os.path.join(base, v["config"])
            pins = validate_config(cfg_path, reg, validated, problems)
            if v.get("aspiration") == "naturally_aspirated":
                cfg = read_json(cfg_path)
                if any(i.get("app") in ("MAP_PRESSURE", "BOOST_PRESSURE") for i in cfg.get("inputs", [])):
                    problems.append(f"{cfg_path}: naturally aspirated config must not include a boost/MAP input")
        if v.get("settings"):
            validate_settings(os.path.join(base, v["settings"]), pins, problems)
    return p


def validate_all(profiles_dir=PROFILES, repo=REPO):
    reg = load_registry(repo)
    problems = []
    found = []
    for root, _dirs, files in os.walk(profiles_dir):
        for f in sorted(files):
            if f.endswith(".profile.json"):
                p = validate_profile(os.path.join(root, f), reg, problems)
                if p:
                    found.append(p["id"])
    if "OM617" not in found:
        problems.append("OM617 profile missing (primary development engine)")
    return sorted(found), problems


def main():
    found, problems = validate_all()
    for pr in problems:
        print("ERROR:", os.path.relpath(pr, REPO) if os.path.isabs(pr) else pr)
    print(f"{len(found)} profiles checked: {', '.join(found)}")
    if problems:
        print(f"{len(problems)} problem(s)")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
