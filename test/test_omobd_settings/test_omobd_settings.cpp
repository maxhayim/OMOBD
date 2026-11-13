// OMOBD native tests: settings defaults, validation, JSON persistence round
// trip and safe fallback on corrupt storage (src/omobd/omobd_settings.cpp).
#include <unity.h>
#include <string.h>
#include <math.h>
#include "../../src/omobd/omobd_derived.cpp"
#include "../../src/omobd/omobd_settings.cpp"

void setUp(void) {}
void tearDown(void) {}

static OmobdSettings validBase() {
    OmobdSettings s;
    omobdSettingsDefaults(s);
    strcpy(s.apPass, "abcdefgh1234");   // firmware generates one on first boot
    return s;
}

static void test_defaults_are_valid_and_conservative(void) {
    OmobdSettings s = validBase();
    char why[64] = "";
    TEST_ASSERT_TRUE_MESSAGE(omobdSettingsValidate(s, why, sizeof(why)), why);
    TEST_ASSERT_EQUAL(OMOBD_WIFI_AP, s.wifiMode);              // works with no internet
    TEST_ASSERT_EQUAL(0, s.oilCurveCount);                     // no invented oil limits
    TEST_ASSERT_EQUAL(OMOBD_PIN_NONE, s.boostMapPin);          // nothing assumed installed
    TEST_ASSERT_EQUAL(OMOBD_PIN_NONE, s.rpmPin);
    TEST_ASSERT_EQUAL(0, s.allowOpenAp);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, OMOBD_STD_ATM_BAR, s.atmFixedBar);
}

static void test_validation_rejects_bad_values(void) {
    char why[64];
    OmobdSettings s = validBase(); strcpy(s.apPass, "short");
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.atmFixedBar = 2.0f;
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.atmSource = OMOBD_ATM_INPUT;            // no input pin
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.atmSource = OMOBD_ATM_KEY_ON_MAP;       // no MAP pin
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.wifiMode = OMOBD_WIFI_STA_FALLBACK_AP;  // no network name
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.oilCurveCount = 1; s.oilCurve[0] = {800, 0.5f};  // curve but no pins
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.historyIntervalMs = 10;
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.rpmPin = 0x90;                           // between physical and CAN ranges
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
    s = validBase(); s.runningRpm = NAN;
    TEST_ASSERT_FALSE(omobdSettingsValidate(s, why, sizeof(why)));
}

static void test_json_round_trip(void) {
    OmobdSettings s = validBase();
    strcpy(s.profileId, "OM617.95x");
    s.boostMapPin = 4; s.vacuumMapPin = 5; s.rpmPin = 0xC1; s.oilPressurePin = 3;
    s.atmSource = OMOBD_ATM_KEY_ON_MAP;
    s.oilCurveCount = 2; s.oilCurve[0] = {800, 0.4f}; s.oilCurve[1] = {3000, 1.8f};
    char buf[2048];
    size_t n = omobdSettingsToJson(s, buf, sizeof(buf), true);
    TEST_ASSERT_TRUE(n > 0);
    OmobdSettings base; omobdSettingsDefaults(base);
    OmobdSettings back;
    char why[64] = "";
    TEST_ASSERT_TRUE_MESSAGE(omobdSettingsFromJson(buf, n, base, back, why, sizeof(why)), why);
    TEST_ASSERT_EQUAL_STRING("OM617.95x", back.profileId);
    TEST_ASSERT_EQUAL(4, back.boostMapPin);
    TEST_ASSERT_EQUAL(0xC1, back.rpmPin);
    TEST_ASSERT_EQUAL(OMOBD_ATM_KEY_ON_MAP, back.atmSource);
    TEST_ASSERT_EQUAL(2, back.oilCurveCount);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.8f, back.oilCurve[1].minBar);
    TEST_ASSERT_EQUAL_STRING(s.apPass, back.apPass);
}

static void test_export_omits_secrets(void) {
    OmobdSettings s = validBase();
    strcpy(s.staPass, "home-network-pw");
    char buf[2048];
    TEST_ASSERT_TRUE(omobdSettingsToJson(s, buf, sizeof(buf), false) > 0);
    TEST_ASSERT_NULL(strstr(buf, "abcdefgh1234"));
    TEST_ASSERT_NULL(strstr(buf, "home-network-pw"));
    TEST_ASSERT_NULL(strstr(buf, "apPass"));
}

static void test_partial_update_keeps_other_fields(void) {
    OmobdSettings base = validBase();
    base.boostMapPin = 4;
    const char* patch = "{\"pins\":{\"vacuumMap\":5}}";
    OmobdSettings out; char why[64];
    TEST_ASSERT_TRUE(omobdSettingsFromJson(patch, strlen(patch), base, out, why, sizeof(why)));
    TEST_ASSERT_EQUAL(4, out.boostMapPin);
    TEST_ASSERT_EQUAL(5, out.vacuumMapPin);
    // null means "not fitted"
    const char* clear = "{\"pins\":{\"boostMap\":null}}";
    TEST_ASSERT_TRUE(omobdSettingsFromJson(clear, strlen(clear), out, out, why, sizeof(why)));
    TEST_ASSERT_EQUAL(OMOBD_PIN_NONE, out.boostMapPin);
}

static void test_invalid_update_leaves_target_untouched(void) {
    OmobdSettings base = validBase();
    OmobdSettings out = base;
    char why[64] = "";
    const char* bad = "{\"atmosphere\":{\"fixedBar\":5.0}}";
    TEST_ASSERT_FALSE(omobdSettingsFromJson(bad, strlen(bad), base, out, why, sizeof(why)));
    TEST_ASSERT_TRUE(strlen(why) > 0);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, base.atmFixedBar, out.atmFixedBar);
    const char* wrongType = "{\"pins\":{\"rpm\":\"A3\"}}";
    TEST_ASSERT_FALSE(omobdSettingsFromJson(wrongType, strlen(wrongType), base, out, why, sizeof(why)));
    const char* badMode = "{\"wifi\":{\"mode\":\"MESH\"}}";
    TEST_ASSERT_FALSE(omobdSettingsFromJson(badMode, strlen(badMode), base, out, why, sizeof(why)));
    const char* tooLong = "{\"wifi\":{\"apSsid\":\"0123456789012345678901234567890123456789\"}}";
    TEST_ASSERT_FALSE(omobdSettingsFromJson(tooLong, strlen(tooLong), base, out, why, sizeof(why)));
    const char* schema = "{\"schema\":99}";
    TEST_ASSERT_FALSE(omobdSettingsFromJson(schema, strlen(schema), base, out, why, sizeof(why)));
}

static void test_corrupt_storage_falls_back_to_defaults(void) {
    OmobdSettings out; char why[64] = "";
    const char* corrupt = "{\"schema\":1,\"profileId\":\"OM6";   // truncated flash write
    TEST_ASSERT_FALSE(omobdSettingsLoadOrDefault(corrupt, strlen(corrupt), out, why, sizeof(why)));
    OmobdSettings d; omobdSettingsDefaults(d);
    TEST_ASSERT_EQUAL_MEMORY(&d, &out, sizeof(d));
    TEST_ASSERT_FALSE(omobdSettingsLoadOrDefault(nullptr, 0, out, why, sizeof(why)));
    TEST_ASSERT_EQUAL_STRING("nothing stored", why);
}

static void test_stored_settings_used_when_valid(void) {
    const char* stored = "{\"schema\":1,\"profileId\":\"OM617.912\",\"wifi\":{\"apPass\":\"abcdefgh\"}}";
    OmobdSettings out; char why[64] = "";
    TEST_ASSERT_TRUE_MESSAGE(omobdSettingsLoadOrDefault(stored, strlen(stored), out, why, sizeof(why)), why);
    TEST_ASSERT_EQUAL_STRING("OM617.912", out.profileId);
}

static void test_oil_curve_via_json(void) {
    OmobdSettings base = validBase();
    base.rpmPin = 6; base.oilPressurePin = 3;
    const char* bad = "{\"oilCurve\":{\"points\":[{\"rpm\":2000,\"minBar\":1},{\"rpm\":1000,\"minBar\":0.5}]}}";
    OmobdSettings out; char why[64];
    TEST_ASSERT_FALSE(omobdSettingsFromJson(bad, strlen(bad), base, out, why, sizeof(why)));
    const char* good = "{\"oilCurve\":{\"points\":[{\"rpm\":800,\"minBar\":0.5},{\"rpm\":3000,\"minBar\":2}]}}";
    TEST_ASSERT_TRUE(omobdSettingsFromJson(good, strlen(good), base, out, why, sizeof(why)));
    TEST_ASSERT_EQUAL(2, out.oilCurveCount);
    const char* clear = "{\"oilCurve\":{\"points\":[]}}";
    TEST_ASSERT_TRUE(omobdSettingsFromJson(clear, strlen(clear), out, out, why, sizeof(why)));
    TEST_ASSERT_EQUAL(0, out.oilCurveCount);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid_and_conservative);
    RUN_TEST(test_validation_rejects_bad_values);
    RUN_TEST(test_json_round_trip);
    RUN_TEST(test_export_omits_secrets);
    RUN_TEST(test_partial_update_keeps_other_fields);
    RUN_TEST(test_invalid_update_leaves_target_untouched);
    RUN_TEST(test_corrupt_storage_falls_back_to_defaults);
    RUN_TEST(test_stored_settings_used_when_valid);
    RUN_TEST(test_oil_curve_via_json);
    return UNITY_END();
}
