/*
 * omobd_settings.cpp - OMOBD device settings. See omobd_settings.h.
 */
// Compiled only into OMOBD builds and host tests; upstream preOBD boards
// get byte-identical firmware.
#if (defined(OMOBD_WEB_DASHBOARD) && OMOBD_WEB_DASHBOARD) || defined(NATIVE_TEST)

#include "omobd_settings.h"
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static void setWhy(char* why, size_t whyLen, const char* msg) {
    if (why && whyLen) {
        strncpy(why, msg, whyLen - 1);
        why[whyLen - 1] = '\0';
    }
}

static void copyStr(char* dst, size_t cap, const char* src) {
    if (!src) src = "";
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

void omobdSettingsDefaults(OmobdSettings& s) {
    memset(&s, 0, sizeof(s));
    s.schema = OMOBD_SETTINGS_SCHEMA;
    copyStr(s.profileId, sizeof(s.profileId), "OM617");
    s.wifiMode = OMOBD_WIFI_AP;
    copyStr(s.apSsid, sizeof(s.apSsid), "OMOBD");
    // The AP password is NOT a published default: omobd_web.cpp derives a
    // per-device one from the chip's MAC on first boot and prints it on the
    // USB console. An empty value here means "not yet generated".
    s.apPass[0] = '\0';
    s.allowOpenAp = 0;
    s.atmSource = OMOBD_ATM_FIXED;
    s.atmFixedBar = OMOBD_STD_ATM_BAR;
    s.atmInputPin = OMOBD_PIN_NONE;
    s.boostMapPin = OMOBD_PIN_NONE;
    s.vacuumMapPin = OMOBD_PIN_NONE;
    s.rpmPin = OMOBD_PIN_NONE;
    s.oilPressurePin = OMOBD_PIN_NONE;
    s.oilCurveCount = 0;           // unconfigured until the owner enters validated values
    s.oilWarmupMs = 10000;
    s.oilDebounceMs = 2000;
    s.runningRpm = 300.0f;         // "engine turning under its own power", not a limit
    s.livePollMs = 500;
    s.historyIntervalMs = 1000;
}

static bool pinOk(uint8_t p) {
    // OMOBD_PIN_NONE, a physical GPIO, or a preOBD virtual pin (CAN:n / I2C:n).
    return p == OMOBD_PIN_NONE || p <= 0x7F || (p >= 0xC0 && p <= 0xDF) || (p >= 0xF0 && p <= 0xFC);
}

bool omobdSettingsValidate(const OmobdSettings& s, char* why, size_t whyLen) {
    if (s.schema != OMOBD_SETTINGS_SCHEMA) { setWhy(why, whyLen, "schema"); return false; }
    if (s.profileId[0] == '\0') { setWhy(why, whyLen, "profileId empty"); return false; }
    if (s.wifiMode > OMOBD_WIFI_STA_FALLBACK_AP) { setWhy(why, whyLen, "wifiMode"); return false; }
    size_t apLen = strnlen(s.apSsid, sizeof(s.apSsid));
    if (s.wifiMode != OMOBD_WIFI_OFF && (apLen == 0 || apLen > 32)) { setWhy(why, whyLen, "apSsid"); return false; }
    size_t pl = strnlen(s.apPass, sizeof(s.apPass));
    if (pl != 0 && (pl < 8 || pl > 63)) { setWhy(why, whyLen, "apPass must be 8-63 chars"); return false; }
    if (s.wifiMode == OMOBD_WIFI_STA_FALLBACK_AP && s.staSsid[0] == '\0') {
        setWhy(why, whyLen, "staSsid required for station mode"); return false;
    }
    if (s.atmSource > OMOBD_ATM_KEY_ON_MAP) { setWhy(why, whyLen, "atmSource"); return false; }
    if (!omobdAtmValid(s.atmFixedBar)) { setWhy(why, whyLen, "atmFixedBar out of 0.5-1.1 bar"); return false; }
    if (s.atmSource == OMOBD_ATM_INPUT && s.atmInputPin == OMOBD_PIN_NONE) {
        setWhy(why, whyLen, "atmInputPin required"); return false;
    }
    if (s.atmSource == OMOBD_ATM_KEY_ON_MAP && s.boostMapPin == OMOBD_PIN_NONE) {
        setWhy(why, whyLen, "key-on reference needs boostMapPin"); return false;
    }
    if (!pinOk(s.atmInputPin) || !pinOk(s.boostMapPin) || !pinOk(s.vacuumMapPin) ||
        !pinOk(s.rpmPin) || !pinOk(s.oilPressurePin)) {
        setWhy(why, whyLen, "pin out of range"); return false;
    }
    if (s.oilCurveCount > OMOBD_OIL_CURVE_MAX) { setWhy(why, whyLen, "oilCurve too long"); return false; }
    if (s.oilCurveCount > 0) {
        if (!omobdOilCurveValid(s.oilCurve, s.oilCurveCount)) {
            setWhy(why, whyLen, "oilCurve must ascend in rpm, 0-10 bar"); return false;
        }
        if (s.rpmPin == OMOBD_PIN_NONE || s.oilPressurePin == OMOBD_PIN_NONE) {
            setWhy(why, whyLen, "oilCurve needs rpmPin and oilPressurePin"); return false;
        }
    }
    if (!isfinite(s.runningRpm) || s.runningRpm < 50.0f || s.runningRpm > 2000.0f) {
        setWhy(why, whyLen, "runningRpm out of 50-2000"); return false;
    }
    if (s.livePollMs < 200 || s.livePollMs > 10000) { setWhy(why, whyLen, "livePollMs 200-10000"); return false; }
    if (s.historyIntervalMs < 250 || s.historyIntervalMs > 60000) {
        setWhy(why, whyLen, "historyIntervalMs 250-60000"); return false;
    }
    return true;
}

static const char* wifiModeName(uint8_t m) {
    switch (m) {
        case OMOBD_WIFI_OFF: return "OFF";
        case OMOBD_WIFI_STA_FALLBACK_AP: return "STA";
        default: return "AP";
    }
}
static int wifiModeFromName(const char* n) {
    if (!n) return -1;
    if (!strcmp(n, "OFF")) return OMOBD_WIFI_OFF;
    if (!strcmp(n, "AP")) return OMOBD_WIFI_AP;
    if (!strcmp(n, "STA")) return OMOBD_WIFI_STA_FALLBACK_AP;
    return -1;
}
static const char* atmName(uint8_t a) {
    switch (a) {
        case OMOBD_ATM_INPUT: return "INPUT";
        case OMOBD_ATM_KEY_ON_MAP: return "KEY_ON_MAP";
        default: return "FIXED";
    }
}
static int atmFromName(const char* n) {
    if (!n) return -1;
    if (!strcmp(n, "FIXED")) return OMOBD_ATM_FIXED;
    if (!strcmp(n, "INPUT")) return OMOBD_ATM_INPUT;
    if (!strcmp(n, "KEY_ON_MAP")) return OMOBD_ATM_KEY_ON_MAP;
    return -1;
}

// Pins travel as numbers; null means "not fitted".
static void putPin(JsonObject o, const char* k, uint8_t p) {
    if (p == OMOBD_PIN_NONE) o[k] = nullptr; else o[k] = p;
}

size_t omobdSettingsToJson(const OmobdSettings& s, char* out, size_t cap, bool includeSecrets) {
    JsonDocument doc;
    doc["schema"] = s.schema;
    doc["profileId"] = s.profileId;
    JsonObject w = doc["wifi"].to<JsonObject>();
    w["mode"] = wifiModeName(s.wifiMode);
    w["apSsid"] = s.apSsid;
    if (includeSecrets) w["apPass"] = s.apPass;
    w["allowOpenAp"] = (bool)s.allowOpenAp;
    w["staSsid"] = s.staSsid;
    if (includeSecrets) w["staPass"] = s.staPass;
    JsonObject a = doc["atmosphere"].to<JsonObject>();
    a["source"] = atmName(s.atmSource);
    a["fixedBar"] = s.atmFixedBar;
    putPin(a, "inputPin", s.atmInputPin);
    JsonObject p = doc["pins"].to<JsonObject>();
    putPin(p, "boostMap", s.boostMapPin);
    putPin(p, "vacuumMap", s.vacuumMapPin);
    putPin(p, "rpm", s.rpmPin);
    putPin(p, "oilPressure", s.oilPressurePin);
    JsonObject oil = doc["oilCurve"].to<JsonObject>();
    JsonArray pts = oil["points"].to<JsonArray>();
    for (uint8_t i = 0; i < s.oilCurveCount && i < OMOBD_OIL_CURVE_MAX; i++) {
        JsonObject pt = pts.add<JsonObject>();
        pt["rpm"] = s.oilCurve[i].rpm;
        pt["minBar"] = s.oilCurve[i].minBar;
    }
    oil["warmupMs"] = s.oilWarmupMs;
    oil["debounceMs"] = s.oilDebounceMs;
    doc["runningRpm"] = s.runningRpm;
    doc["livePollMs"] = s.livePollMs;
    doc["historyIntervalMs"] = s.historyIntervalMs;
    size_t need = measureJson(doc);
    if (need + 1 > cap) return 0;
    return serializeJson(doc, out, cap);
}

static bool readPin(JsonVariantConst v, uint8_t& dst) {
    if (v.isUnbound()) return true;           // key absent: keep
    if (v.isNull()) { dst = OMOBD_PIN_NONE; return true; }
    if (!v.is<int>()) return false;
    int n = v.as<int>();
    if (n < 0 || n > 0xFE) return false;
    dst = (uint8_t)n;
    return true;
}

template <typename T>
static bool readNum(JsonVariantConst v, T& dst) {
    if (v.isUnbound()) return true;
    if (!v.is<float>() && !v.is<int>()) return false;
    dst = v.as<T>();
    return true;
}

static bool readStr(JsonVariantConst v, char* dst, size_t cap) {
    if (v.isUnbound()) return true;
    if (!v.is<const char*>()) return false;
    const char* s = v.as<const char*>();
    if (strlen(s) >= cap) return false;
    copyStr(dst, cap, s);
    return true;
}

bool omobdSettingsFromJson(const char* json, size_t len, const OmobdSettings& base,
                           OmobdSettings& out, char* why, size_t whyLen) {
    if (!json || len == 0) { setWhy(why, whyLen, "empty"); return false; }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, len);
    if (err) { setWhy(why, whyLen, "invalid JSON"); return false; }
    if (!doc.is<JsonObject>()) { setWhy(why, whyLen, "not an object"); return false; }

    OmobdSettings s = base;
    JsonVariantConst schema = doc["schema"];
    if (!schema.isUnbound() && schema.as<int>() != OMOBD_SETTINGS_SCHEMA) {
        setWhy(why, whyLen, "unsupported schema"); return false;
    }
    bool ok = readStr(doc["profileId"], s.profileId, sizeof(s.profileId));

    JsonVariantConst w = doc["wifi"];
    if (!w.isUnbound()) {
        JsonVariantConst m = w["mode"];
        if (!m.isUnbound()) {
            int mode = wifiModeFromName(m.as<const char*>());
            if (mode < 0) { setWhy(why, whyLen, "wifi.mode"); return false; }
            s.wifiMode = (uint8_t)mode;
        }
        ok = ok && readStr(w["apSsid"], s.apSsid, sizeof(s.apSsid));
        ok = ok && readStr(w["apPass"], s.apPass, sizeof(s.apPass));
        ok = ok && readStr(w["staSsid"], s.staSsid, sizeof(s.staSsid));
        ok = ok && readStr(w["staPass"], s.staPass, sizeof(s.staPass));
        if (!w["allowOpenAp"].isUnbound()) s.allowOpenAp = w["allowOpenAp"].as<bool>() ? 1 : 0;
    }
    JsonVariantConst a = doc["atmosphere"];
    if (!a.isUnbound()) {
        JsonVariantConst src = a["source"];
        if (!src.isUnbound()) {
            int as = atmFromName(src.as<const char*>());
            if (as < 0) { setWhy(why, whyLen, "atmosphere.source"); return false; }
            s.atmSource = (uint8_t)as;
        }
        ok = ok && readNum(a["fixedBar"], s.atmFixedBar);
        ok = ok && readPin(a["inputPin"], s.atmInputPin);
    }
    JsonVariantConst p = doc["pins"];
    if (!p.isUnbound()) {
        ok = ok && readPin(p["boostMap"], s.boostMapPin);
        ok = ok && readPin(p["vacuumMap"], s.vacuumMapPin);
        ok = ok && readPin(p["rpm"], s.rpmPin);
        ok = ok && readPin(p["oilPressure"], s.oilPressurePin);
    }
    JsonVariantConst oil = doc["oilCurve"];
    if (!oil.isUnbound()) {
        JsonVariantConst pts = oil["points"];
        if (!pts.isUnbound()) {
            if (!pts.is<JsonArrayConst>()) { setWhy(why, whyLen, "oilCurve.points"); return false; }
            JsonArrayConst arr = pts.as<JsonArrayConst>();
            if (arr.size() > OMOBD_OIL_CURVE_MAX) { setWhy(why, whyLen, "oilCurve too long"); return false; }
            uint8_t n = 0;
            for (JsonVariantConst pt : arr) {
                if (!pt["rpm"].is<float>() || !pt["minBar"].is<float>()) {
                    setWhy(why, whyLen, "oilCurve point needs rpm and minBar"); return false;
                }
                s.oilCurve[n].rpm = pt["rpm"].as<float>();
                s.oilCurve[n].minBar = pt["minBar"].as<float>();
                n++;
            }
            for (uint8_t i = n; i < OMOBD_OIL_CURVE_MAX; i++) s.oilCurve[i] = OmobdOilPoint{0, 0};
            s.oilCurveCount = n;
        }
        ok = ok && readNum(oil["warmupMs"], s.oilWarmupMs);
        ok = ok && readNum(oil["debounceMs"], s.oilDebounceMs);
    }
    ok = ok && readNum(doc["runningRpm"], s.runningRpm);
    ok = ok && readNum(doc["livePollMs"], s.livePollMs);
    ok = ok && readNum(doc["historyIntervalMs"], s.historyIntervalMs);
    if (!ok) { setWhy(why, whyLen, "field has wrong type or is too long"); return false; }

    if (!omobdSettingsValidate(s, why, whyLen)) return false;
    out = s;
    return true;
}

bool omobdSettingsLoadOrDefault(const char* stored, size_t len, OmobdSettings& out,
                                char* why, size_t whyLen) {
    OmobdSettings defaults;
    omobdSettingsDefaults(defaults);
    if (stored && len && omobdSettingsFromJson(stored, len, defaults, out, why, whyLen)) {
        return true;
    }
    if (!stored || !len) setWhy(why, whyLen, "nothing stored");
    out = defaults;
    return false;
}

#endif // OMOBD build guard
