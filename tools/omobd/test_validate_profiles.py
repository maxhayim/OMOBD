#!/usr/bin/env python3
"""Unit tests for validate_profiles.py.  Run: python3 -m unittest discover tools/omobd"""
import json
import os
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import validate_profiles as vp  # noqa: E402


class RealProfiles(unittest.TestCase):
    def test_repository_profiles_are_valid(self):
        found, problems = vp.validate_all()
        self.assertEqual(problems, [])
        for eid in ("OM615", "OM616", "OM617", "OM601", "OM602", "OM603", "OM604", "OM605", "OM606"):
            self.assertIn(eid, found)

    def test_om617_supports_na_and_turbo(self):
        p = vp.read_json(os.path.join(vp.PROFILES, "OM617", "OM617.profile.json"))
        asp = {v["aspiration"] for v in p["variants"]}
        self.assertEqual(asp, {"naturally_aspirated", "turbocharged"})

    def test_no_threshold_values_shipped(self):
        for root, _d, files in os.walk(vp.PROFILES):
            for f in files:
                if f.endswith(".profile.json"):
                    p = vp.read_json(os.path.join(root, f))
                    for k, t in p["thresholds"].items():
                        if not k.startswith("_"):
                            self.assertIsNone(t["value"], f"{f}:{k}")

    def test_nothing_claims_tested(self):
        for root, _d, files in os.walk(vp.PROFILES):
            for f in files:
                if f.endswith(".profile.json"):
                    p = vp.read_json(os.path.join(root, f))
                    for k, c in p["capabilities"].items():
                        self.assertNotEqual(c["status"], "tested", f"{f}:{k}")


class BrokenProfiles(unittest.TestCase):
    """Copy the OM617 profile into a temp tree, break it, expect problems."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        shutil.copytree(os.path.join(vp.PROFILES, "OM617"), os.path.join(self.tmp, "OM617"))
        self.path = os.path.join(self.tmp, "OM617", "OM617.profile.json")
        self.cfg = os.path.join(self.tmp, "OM617", "configs", "esp32s3-om617-turbo.preobd.json")

    def tearDown(self):
        shutil.rmtree(self.tmp)

    def edit(self, path, fn):
        d = vp.read_json(path)
        fn(d)
        with open(path, "w") as fh:
            json.dump(d, fh)

    def problems(self):
        return vp.validate_all(self.tmp, vp.REPO)[1]

    def test_clean_copy_passes(self):
        self.assertEqual(self.problems(), [])

    def test_unvalidated_threshold_value_rejected(self):
        self.edit(self.path, lambda d: d["thresholds"]["egtPreTurboMaxC"].update(value=650))
        self.assertTrue(any("not 'validated'" in p for p in self.problems()))

    def test_validated_threshold_needs_source(self):
        self.edit(self.path, lambda d: d["thresholds"]["egtPreTurboMaxC"].update(value=650, status="validated"))
        self.assertTrue(any("not 'validated' with a source" in p for p in self.problems()))

    def test_unknown_sensor_rejected(self):
        self.edit(self.path, lambda d: d["capabilities"]["egt"]["sensors"].append("MAX99999"))
        self.assertTrue(any("unknown sensor MAX99999" in p for p in self.problems()))

    def test_tested_needs_evidence(self):
        self.edit(self.path, lambda d: d["capabilities"]["egt"].update(status="tested"))
        self.assertTrue(any("testedOn" in p for p in self.problems()))

    def test_rules_in_config_rejected_without_validated_thresholds(self):
        self.edit(self.cfg, lambda d: d.update(rules=[{"id": 0, "input": 14, "condition": "ABOVE",
                                                       "severity": "ALARM", "limitLo": 600}]))
        self.assertTrue(any("ships alarm rules" in p for p in self.problems()))

    def test_missing_rules_array_rejected(self):
        self.edit(self.cfg, lambda d: d.pop("rules"))
        self.assertTrue(any("explicit 'rules'" in p for p in self.problems()))

    def test_wrong_analog_label_rejected(self):
        def bad(d):
            d["inputs"][0]["pinLabel"] = "A5"
        self.edit(self.cfg, bad)
        self.assertTrue(any("label A5" in p for p in self.problems()))

    def test_unknown_app_in_config_rejected(self):
        def bad(d):
            d["inputs"][0]["app"] = "FLUX_CAPACITOR"
        self.edit(self.cfg, bad)
        self.assertTrue(any("unknown application FLUX_CAPACITOR" in p for p in self.problems()))

    def test_boost_input_in_na_config_rejected(self):
        na = os.path.join(self.tmp, "OM617", "configs", "esp32s3-om617-na.preobd.json")
        def bad(d):
            d["inputs"].append({"idx": 20, "pin": 16, "app": "MAP_PRESSURE", "sensor": "MPX4250AP", "units": "BAR"})
        self.edit(na, bad)
        self.assertTrue(any("naturally aspirated" in p for p in self.problems()))

    def test_settings_pin_must_exist_in_config(self):
        st = os.path.join(self.tmp, "OM617", "configs", "esp32s3-om617-turbo.omobd-settings.json")
        self.edit(st, lambda d: d["pins"].update(rpm=33))
        self.assertTrue(any("pins.rpm = 33" in p for p in self.problems()))

    def test_shipped_oil_curve_rejected(self):
        st = os.path.join(self.tmp, "OM617", "configs", "esp32s3-om617-turbo.omobd-settings.json")
        self.edit(st, lambda d: d["oilCurve"].update(points=[{"rpm": 800, "minBar": 0.5}]))
        self.assertTrue(any("oilCurve must ship empty" in p for p in self.problems()))


if __name__ == "__main__":
    unittest.main()
