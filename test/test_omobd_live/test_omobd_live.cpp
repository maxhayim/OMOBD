// OMOBD native tests: /api/live document, history ring, and a mock sensor
// stream driven through the calculated values (no hardware).
#include <unity.h>
#include <string.h>
#include <math.h>
#include <ArduinoJson.h>
#include "../../src/omobd/omobd_derived.cpp"
#include "../../src/omobd/omobd_live.cpp"
#include "../../src/omobd/omobd_history.h"

void setUp(void) {}
void tearDown(void) {}

static OmobdLiveMeta meta() {
    OmobdLiveMeta m;
    m.uptimeMs = 12345; m.mode = "RUN"; m.systemSeverity = 0;
    m.profileId = "OM617"; m.firmware = "test"; m.seq = 7;
    return m;
}

static OmobdLiveDerived derived() {
    OmobdLiveDerived d;
    d.atmBar = 1.01325f; d.atmSource = "FIXED";
    d.boostBar = 0.62f; d.vacuumBar = NAN; d.engineHours = NAN;
    d.oilState = "unconfigured"; d.oilMinBar = NAN;
    return d;
}

static void test_live_document_parses_and_marks_sources(void) {
    OmobdLiveChannel ch[2] = {
        {2, "EGT", "EGT", "Exhaust Gas Temperature", "C", 520.0f, 520.0f,
         OMOBD_HEALTH_OK, OMOBD_SRC_MEASURED, 0},
        {6, "OIL_PRESSURE", "OIL", "Oil \"main\" gallery", "bar", NAN, NAN,
         OMOBD_HEALTH_FAULT, OMOBD_SRC_MEASURED, 3},
    };
    char buf[4096];
    size_t n = omobdWriteLiveJson(buf, sizeof(buf), meta(), ch, 2, derived());
    TEST_ASSERT_TRUE(n > 0);
    JsonDocument doc;
    TEST_ASSERT_FALSE_MESSAGE(deserializeJson(doc, buf), buf);
    TEST_ASSERT_EQUAL(7, doc["seq"].as<int>());
    TEST_ASSERT_EQUAL_STRING("OM617", doc["profile"]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 520.0f, doc["channels"][0]["value"].as<float>());
    TEST_ASSERT_EQUAL_STRING("measured", doc["channels"][0]["src"]);
    // A faulted sensor is null, never 0, and keeps its alarm severity.
    TEST_ASSERT_TRUE(doc["channels"][1]["value"].isNull());
    TEST_ASSERT_EQUAL_STRING("fault", doc["channels"][1]["health"]);
    TEST_ASSERT_EQUAL(3, doc["channels"][1]["severity"].as<int>());
    TEST_ASSERT_EQUAL_STRING("Oil \"main\" gallery", doc["channels"][1]["label"]);
    // Derived values are labelled calculated; missing ones are null.
    TEST_ASSERT_EQUAL_STRING("calculated", doc["derived"]["boost"]["src"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.62f, doc["derived"]["boost"]["bar"].as<float>());
    TEST_ASSERT_TRUE(doc["derived"]["vacuum"]["bar"].isNull());
    TEST_ASSERT_TRUE(doc["derived"]["engineHours"]["hours"].isNull());
}

static void test_no_reading_and_disabled_are_null(void) {
    OmobdLiveChannel ch[2] = {
        {1, "COOLANT_TEMP", "ECT", "Coolant", "C", 0.0f, 0.0f, OMOBD_HEALTH_NO_READING, OMOBD_SRC_MEASURED, 0},
        {3, "OIL_TEMP", "OIL", "Oil", "C", 88.0f, 88.0f, OMOBD_HEALTH_DISABLED, OMOBD_SRC_MEASURED, 0},
    };
    char buf[4096];
    TEST_ASSERT_TRUE(omobdWriteLiveJson(buf, sizeof(buf), meta(), ch, 2, derived()) > 0);
    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, buf));
    TEST_ASSERT_TRUE(doc["channels"][0]["value"].isNull());   // stale 0.0 never shown
    TEST_ASSERT_TRUE(doc["channels"][1]["value"].isNull());
}

static void test_out_of_range_value_still_shown_but_flagged(void) {
    OmobdLiveChannel ch[1] = {
        {2, "EGT", "EGT", "EGT", "C", 1400.0f, 1400.0f, OMOBD_HEALTH_OUT_OF_RANGE, OMOBD_SRC_MEASURED, 0},
    };
    char buf[2048];
    TEST_ASSERT_TRUE(omobdWriteLiveJson(buf, sizeof(buf), meta(), ch, 1, derived()) > 0);
    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, buf));
    TEST_ASSERT_EQUAL_STRING("out_of_range", doc["channels"][0]["health"]);
    TEST_ASSERT_FALSE(doc["channels"][0]["value"].isNull());
}

static void test_small_buffer_returns_zero_not_truncated_json(void) {
    OmobdLiveChannel ch[1] = {
        {2, "EGT", "EGT", "EGT", "C", 400.0f, 400.0f, OMOBD_HEALTH_OK, OMOBD_SRC_MEASURED, 0},
    };
    char buf[64];
    TEST_ASSERT_EQUAL(0, omobdWriteLiveJson(buf, sizeof(buf), meta(), ch, 1, derived()));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

static void test_json_escape(void) {
    char out[32];
    TEST_ASSERT_TRUE(omobdJsonEscape("a\"b\\c\n", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("a\\\"b\\\\c\\n", out);
    TEST_ASSERT_TRUE(omobdJsonEscape("\x01", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("\\u0001", out);
    char tiny[4];
    TEST_ASSERT_FALSE(omobdJsonEscape("abcdef", tiny, sizeof(tiny)));
}

static void test_history_ring(void) {
    OmobdHistoryRing<4> r;
    TEST_ASSERT_EQUAL(0, r.size());
    TEST_ASSERT_TRUE(isnan(r.newest()));
    for (int i = 1; i <= 6; i++) r.push((float)i);
    TEST_ASSERT_EQUAL(4, r.size());
    TEST_ASSERT_EQUAL_FLOAT(3.0f, r.at(0));   // oldest kept
    TEST_ASSERT_EQUAL_FLOAT(6.0f, r.newest());
    r.push(NAN);                               // gaps are kept, not hidden
    TEST_ASSERT_TRUE(isnan(r.newest()));
    r.clear();
    TEST_ASSERT_EQUAL(0, r.size());
}

// Mock stream: a cold start, a boost pull and a MAP sensor that drops out.
// Checks the calculated boost follows the stream and goes null on a fault.
static void test_mock_stream_boost_and_fault(void) {
    struct Sample { float rpm; float mapAbs; };
    const Sample stream[] = {
        {0, 1.01f}, {780, 1.00f}, {1500, 1.10f}, {2800, 1.70f}, {3200, 1.82f},
        {3200, NAN}, {3000, NAN}, {2000, 1.30f},
    };
    OmobdHistoryRing<16> boostHist;
    float atm = OMOBD_STD_ATM_BAR;
    float peak = -10.0f;
    int faults = 0;
    for (const Sample& s : stream) {
        float b = omobdGaugeFromAbsolute(s.mapAbs, atm);
        boostHist.push(b);
        if (isnan(b)) faults++; else if (b > peak) peak = b;
    }
    TEST_ASSERT_EQUAL(8, boostHist.size());
    TEST_ASSERT_EQUAL(2, faults);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.82f - OMOBD_STD_ATM_BAR, peak);
    TEST_ASSERT_TRUE(isnan(boostHist.at(5)));
    TEST_ASSERT_FALSE(isnan(boostHist.newest()));
}

// Mock stream through the health classifier: a thermocouple that opens.
static void test_mock_stream_thermocouple_open(void) {
    const float egt[] = {25, 180, 320, 450, NAN, NAN, 440};
    const OmobdHealth want[] = {OMOBD_HEALTH_OK, OMOBD_HEALTH_OK, OMOBD_HEALTH_OK, OMOBD_HEALTH_OK,
                                OMOBD_HEALTH_FAULT, OMOBD_HEALTH_FAULT, OMOBD_HEALTH_OK};
    for (int i = 0; i < 7; i++) {
        // MAX31855 K-type reportable range in preOBD's library is wide; use 0-1250 here.
        TEST_ASSERT_EQUAL(want[i], omobdClassifyInput(true, true, egt[i], -200.0f, 1350.0f, 0.05f));
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_live_document_parses_and_marks_sources);
    RUN_TEST(test_no_reading_and_disabled_are_null);
    RUN_TEST(test_out_of_range_value_still_shown_but_flagged);
    RUN_TEST(test_small_buffer_returns_zero_not_truncated_json);
    RUN_TEST(test_json_escape);
    RUN_TEST(test_history_ring);
    RUN_TEST(test_mock_stream_boost_and_fault);
    RUN_TEST(test_mock_stream_thermocouple_open);
    return UNITY_END();
}
