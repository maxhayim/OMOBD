// OMOBD native test: engine-profile configs load through preOBD's JSON IMPORT.
// Scaffolding copied from test_json_import_restore (shared stubs).
#include <unity.h>
#include <Arduino.h>
#include <string.h>

#include "../../src/sensors/sensor_library.cpp"
#include "../../src/sensors/sensor_library/sensor_categories.cpp"
#include "../../src/sensors/units_registry.cpp"
#include "../../src/sensors/application_presets.cpp"
#include "../../src/inputs/input_manager.cpp"
#include "../../src/storage/input_store.cpp"
#include "../../src/inputs/calibration_apply.cpp"
#include "../../src/inputs/measurement_compat.cpp"
#include "../../src/sensors/can_sensor_library/standard_pids.cpp"
#include "../../src/sensors/can_sensor_library/obd_output_pids.cpp"
#include "../../src/pinctrl/pin_label.cpp"
#include "../../src/storage/json_config.cpp"

#include "../stubs/firmware_stubs.h"

// Control engine and config generation: not in this binary. The import calls
// them on a rules/actions array, and since #562 on an inputs array (re-arm).
void configGenerationBump() {}
// obd_query.cpp is not in this binary; count the rebuilds instead (#562).
static int g_obdTableBuilds = 0;
void obdQuery_buildLookupTable() { g_obdTableBuilds++; }
#if INCLUDE_RULES_ENGINE
Action actions[MAX_ACTIONS];
ActionLink actionLinks[MAX_ACTION_LINKS];
static int g_rulesInitCalls = 0;
bool rulesImportRule(JsonObjectConst, Rule&) { return false; }
bool rulesImportAction(JsonObjectConst, Action&, uint8_t*, uint8_t, uint8_t*) { return false; }
bool rulesActionAddRule(uint8_t, uint8_t, bool*) { return false; }
uint8_t rulesSweepActionRules() { return 0; }
void rulesManagerInit(uint32_t) { g_rulesInitCalls++; }
void rulesActionsInit() {}
uint8_t rulesSyncActionPins() { return 0; }
#endif

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

static void resetInputs() {
    for (uint8_t i = 0; i < MAX_INPUTS; i++) {
        memset(&inputs[i], 0, sizeof(Input));
        inputs[i].pin = 0xFF;
    }
    numActiveInputs = 0;
}


// ---------------------------------------------------------------------------
// OMOBD: the OM617 profile configs import through preOBD's own JSON importer
// on a board shaped like the ESP32-S3 (A0..A9 = GPIO 1..10, GPIO up to 48).
// ---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

void setUp(void) {
    memset(g_pinReserved, 0, sizeof(g_pinReserved));
    resetInputs();
    inputsTableIsLive = true;
    g_stubDigitalPinMax = 48;
    g_stubAnalogFirst = 1;
    g_stubAnalogCount = 10;
}
void tearDown(void) {}

static char* readRepoFile(const char* rel) {
    // __FILE__ is test/test_omobd_profile_import/<this>.cpp
    char path[512];
    const char* f = __FILE__;
    const char* cut = strstr(f, "test/test_omobd_profile_import/");
    size_t prefix = cut ? (size_t)(cut - f) : 0;
    snprintf(path, sizeof(path), "%.*s%s", (int)prefix, f, rel);
    FILE* fp = fopen(path, "rb");
    if (!fp) return nullptr;
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)n + 1);
    size_t got = fread(buf, 1, (size_t)n, fp);
    buf[got] = '\0';
    fclose(fp);
    return buf;
}

static const char* appName(const Input* in) {
    const ApplicationPreset* a = getApplicationByIndex(in->applicationIndex);
    return a ? READ_APP_NAME(a) : "";
}

static void importFile(const char* rel) {
    char* json = readRepoFile(rel);
    TEST_ASSERT_NOT_NULL_MESSAGE(json, rel);
    TEST_ASSERT_TRUE_MESSAGE(loadConfigFromJSON(json), rel);
    free(json);
}

static void test_om617_turbo_config_imports(void) {
    importFile("engine-profiles/OM617/configs/esp32s3-om617-turbo.preobd.json");
    const Input* map = getInputByPin(4);
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_STRING("MAP_PRESSURE", appName(map));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.6f, map->divider_ratio);
    const Input* vac = getInputByPin(5);
    TEST_ASSERT_NOT_NULL(vac);
    TEST_ASSERT_EQUAL_STRING("VACUUM", appName(vac));
    const Input* egt = getInputByPin(14);
    TEST_ASSERT_NOT_NULL(egt);
    TEST_ASSERT_EQUAL_STRING("EGT", appName(egt));
    const Input* rpm = getInputByPin(15);
    TEST_ASSERT_NOT_NULL(rpm);
    TEST_ASSERT_EQUAL_STRING("ENGINE_RPM", appName(rpm));
    // optional inputs arrive switched off
    const Input* trans = getInputByPin(7);
    TEST_ASSERT_NOT_NULL(trans);
    TEST_ASSERT_FALSE(trans->flags.isEnabled);
    uint8_t occupied = 0;
    for (uint8_t i = 0; i < MAX_INPUTS; i++) if (inputs[i].pin != 0xFF) occupied++;
    TEST_ASSERT_EQUAL_UINT8(12, occupied);   // every input in the file, none dropped
}

static void test_om617_na_config_has_no_boost_input(void) {
    importFile("engine-profiles/OM617/configs/esp32s3-om617-na.preobd.json");
    for (uint8_t i = 0; i < MAX_INPUTS; i++) {
        if (inputs[i].pin == 0xFF) continue;
        TEST_ASSERT_NOT_EQUAL(0, strcmp("MAP_PRESSURE", appName(&inputs[i])));
        TEST_ASSERT_NOT_EQUAL(0, strcmp("BOOST_PRESSURE", appName(&inputs[i])));
    }
    TEST_ASSERT_NOT_NULL(getInputByPin(5));    // vacuum still monitored on NA
}

static void test_profile_config_refused_on_wrong_board(void) {
    // A board without A9 (fewer analog pins): preOBD refuses the whole file and
    // changes nothing — the safe outcome for a mismatched profile.
    g_stubAnalogCount = 4;
    char* json = readRepoFile("engine-profiles/OM617/configs/esp32s3-om617-turbo.preobd.json");
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_FALSE(loadConfigFromJSON(json));
    free(json);
    for (uint8_t i = 0; i < MAX_INPUTS; i++) TEST_ASSERT_EQUAL_UINT8(0xFF, inputs[i].pin);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_om617_turbo_config_imports);
    RUN_TEST(test_om617_na_config_has_no_boost_input);
    RUN_TEST(test_profile_config_refused_on_wrong_board);
    return UNITY_END();
}
