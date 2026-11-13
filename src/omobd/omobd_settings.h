/*
 * omobd_settings.h - OMOBD device settings (pure logic + JSON codec)
 *
 * These are OMOBD's own settings — Wi-Fi, engine profile id, which preOBD
 * inputs feed the calculated values, the oil-pressure-vs-RPM curve. They are
 * stored separately from preOBD's EEPROM configuration (ESP32 NVS, see
 * omobd_web.cpp) so preOBD's storage layout and its SAVE/LOAD/JSON IMPORT
 * behaviour are untouched.
 *
 * Every load is validated. Anything that fails validation is replaced by the
 * defaults and the caller is told, so a corrupt flash page can never leave the
 * device half-configured (safe startup).
 */
#ifndef OMOBD_SETTINGS_H
#define OMOBD_SETTINGS_H

#include <stdint.h>
#include <stddef.h>
#include "omobd_derived.h"

static constexpr uint16_t OMOBD_SETTINGS_SCHEMA = 1;
static constexpr uint8_t  OMOBD_PIN_NONE = 0xFF;

enum OmobdWifiMode : uint8_t {
    OMOBD_WIFI_OFF = 0,
    OMOBD_WIFI_AP,              // device makes its own network (default: works with no internet)
    OMOBD_WIFI_STA_FALLBACK_AP, // join a network; start the AP if that fails
};

enum OmobdAtmSource : uint8_t {
    OMOBD_ATM_FIXED = 0,        // use atmFixedBar
    OMOBD_ATM_INPUT,            // use a preOBD input (BAROMETRIC_PRESSURE, e.g. BME280)
    OMOBD_ATM_KEY_ON_MAP,       // capture the boost MAP sensor at key-on, engine stopped
};

struct OmobdSettings {
    uint16_t schema;

    char     profileId[32];     // e.g. "OM617.952" — must exist in engine-profiles/

    // Wi-Fi
    uint8_t  wifiMode;          // OmobdWifiMode
    char     apSsid[33];
    char     apPass[64];        // empty = open network (refused unless allowOpenAp)
    uint8_t  allowOpenAp;
    char     staSsid[33];
    char     staPass[64];

    // Atmospheric reference for boost/vacuum
    uint8_t  atmSource;         // OmobdAtmSource
    float    atmFixedBar;
    uint8_t  atmInputPin;       // preOBD input pin when atmSource == INPUT

    // Which preOBD inputs feed OMOBD's calculated values (OMOBD_PIN_NONE = not fitted)
    uint8_t  boostMapPin;       // absolute MAP sensor on the intake manifold
    uint8_t  vacuumMapPin;      // absolute sensor teed into the vacuum system
    uint8_t  rpmPin;
    uint8_t  oilPressurePin;

    // Oil pressure vs RPM (advisory). count == 0 = not configured.
    uint8_t       oilCurveCount;
    OmobdOilPoint oilCurve[OMOBD_OIL_CURVE_MAX];
    uint16_t      oilWarmupMs;
    uint16_t      oilDebounceMs;

    // Engine running threshold (hour meter, oil monitor)
    float    runningRpm;

    // Dashboard / history
    uint16_t livePollMs;        // suggested browser poll period
    uint16_t historyIntervalMs; // device-side history sample period
};

void omobdSettingsDefaults(OmobdSettings& s);

// Field-level validation. On failure writes a short reason into `why`
// (may be nullptr) and returns false.
bool omobdSettingsValidate(const OmobdSettings& s, char* why, size_t whyLen);

// JSON codec (ArduinoJson). Serialize never writes Wi-Fi passwords unless
// includeSecrets is true (the export endpoint leaves them out).
size_t omobdSettingsToJson(const OmobdSettings& s, char* out, size_t cap, bool includeSecrets);

// Parse + validate. Fields missing from the JSON keep the value in `base`
// (so a partial update works). On any error `out` is left unchanged.
bool omobdSettingsFromJson(const char* json, size_t len, const OmobdSettings& base,
                           OmobdSettings& out, char* why, size_t whyLen);

// Load path used at boot: parse stored JSON; on failure fall back to defaults.
// Returns true when the stored copy was used, false when defaults were applied.
bool omobdSettingsLoadOrDefault(const char* stored, size_t len, OmobdSettings& out,
                                char* why, size_t whyLen);

#endif // OMOBD_SETTINGS_H
