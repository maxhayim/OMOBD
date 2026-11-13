// OMOBD native tests: calculated values, input health, oil-vs-RPM monitor,
// engine hours. Pure logic from src/omobd/omobd_derived.cpp.
#include <unity.h>
#include <math.h>
#include "../../src/omobd/omobd_derived.cpp"

void setUp(void) {}
void tearDown(void) {}

// ---- pressure ---------------------------------------------------------------

static void test_gauge_from_absolute(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.8f, omobdGaugeFromAbsolute(1.81325f, OMOBD_STD_ATM_BAR));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -0.3f, omobdGaugeFromAbsolute(0.71325f, OMOBD_STD_ATM_BAR));
}

static void test_vacuum_from_absolute(void) {
    // 0.3 bar absolute in a vacuum line at sea level = 0.71325 bar of vacuum
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.71325f, omobdVacuumFromAbsolute(0.3f, OMOBD_STD_ATM_BAR));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 21.06f, omobdBarToInHg(0.71325f));
}

static void test_pressure_nan_and_bad_reference(void) {
    TEST_ASSERT_TRUE(isnan(omobdGaugeFromAbsolute(NAN, 1.0f)));
    TEST_ASSERT_TRUE(isnan(omobdGaugeFromAbsolute(1.5f, NAN)));
    TEST_ASSERT_TRUE(isnan(omobdGaugeFromAbsolute(1.5f, 0.2f)));   // implausible atmosphere
    TEST_ASSERT_TRUE(isnan(omobdVacuumFromAbsolute(0.5f, 2.0f)));
}

static void test_altitude_reference_changes_boost(void) {
    // Same manifold pressure reads as more boost at altitude — the reason the
    // reference is configurable.
    float seaLevel = omobdGaugeFromAbsolute(1.6f, 1.013f);
    float denver   = omobdGaugeFromAbsolute(1.6f, 0.83f);
    TEST_ASSERT_TRUE(denver > seaLevel);
}

static void test_unit_conversions(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 14.50f, omobdBarToPsi(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, omobdBarToKpa(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 29.53f, omobdBarToInHg(1.0f));
}

// ---- input health -------------------------------------------------------------

static void test_health_states(void) {
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_DISABLED, omobdClassifyInput(false, true, 50.0f, 0, 100, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_NO_READING, omobdClassifyInput(true, false, 0.0f, 0, 100, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_FAULT, omobdClassifyInput(true, true, NAN, 0, 100, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_FAULT, omobdClassifyInput(true, true, INFINITY, 0, 100, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_OK, omobdClassifyInput(true, true, 50.0f, 0, 100, 0.05f));
    // margin: 5 % of a 0-100 span = 5
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_OK, omobdClassifyInput(true, true, 104.9f, 0, 100, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_OUT_OF_RANGE, omobdClassifyInput(true, true, 105.1f, 0, 100, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_OUT_OF_RANGE, omobdClassifyInput(true, true, -6.0f, 0, 100, 0.05f));
}

static void test_health_unknown_range_only_flags_nan(void) {
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_OK, omobdClassifyInput(true, true, 99999.0f, 0, 0, 0.05f));
    TEST_ASSERT_EQUAL(OMOBD_HEALTH_OK, omobdClassifyInput(true, true, 5.0f, NAN, NAN, 0.05f));
}

static void test_health_names(void) {
    TEST_ASSERT_EQUAL_STRING("fault", omobdHealthName(OMOBD_HEALTH_FAULT));
    TEST_ASSERT_EQUAL_STRING("calculated", omobdSourceName(OMOBD_SRC_CALCULATED));
}

// ---- oil vs RPM -------------------------------------------------------------
// Curve values here are TEST NUMBERS ONLY, not OM617 specifications.

static const OmobdOilPoint CURVE[] = {{800.0f, 0.5f}, {2000.0f, 1.5f}, {3000.0f, 2.0f}};

static void test_oil_curve_interpolation(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, omobdOilMinForRpm(CURVE, 3, 500.0f));   // held below
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, omobdOilMinForRpm(CURVE, 3, 1400.0f));  // midpoint
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.75f, omobdOilMinForRpm(CURVE, 3, 2500.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, omobdOilMinForRpm(CURVE, 3, 4500.0f));  // held above
    TEST_ASSERT_TRUE(isnan(omobdOilMinForRpm(CURVE, 0, 1000.0f)));
    TEST_ASSERT_TRUE(isnan(omobdOilMinForRpm(CURVE, 3, NAN)));
}

static void test_oil_curve_validation(void) {
    TEST_ASSERT_TRUE(omobdOilCurveValid(CURVE, 3));
    OmobdOilPoint descending[] = {{2000, 1.0f}, {1000, 0.5f}};
    TEST_ASSERT_FALSE(omobdOilCurveValid(descending, 2));
    OmobdOilPoint dup[] = {{1000, 1.0f}, {1000, 0.5f}};
    TEST_ASSERT_FALSE(omobdOilCurveValid(dup, 2));
    OmobdOilPoint silly[] = {{1000, 50.0f}};
    TEST_ASSERT_FALSE(omobdOilCurveValid(silly, 1));
    OmobdOilPoint neg[] = {{-5, 1.0f}};
    TEST_ASSERT_FALSE(omobdOilCurveValid(neg, 1));
    TEST_ASSERT_FALSE(omobdOilCurveValid(CURVE, 0));
}

static OmobdOilConfig cfg() { return OmobdOilConfig{CURVE, 3, 300.0f, 5000, 2000}; }

static void test_oil_unconfigured_judges_nothing(void) {
    OmobdOilMonitor m; omobdOilMonitorReset(m);
    OmobdOilConfig c{CURVE, 0, 300.0f, 0, 0};
    TEST_ASSERT_EQUAL(OMOBD_OIL_UNCONFIGURED, omobdOilMonitorUpdate(m, c, 2000.0f, 0.0f, 1000));
}

static void test_oil_stopped_settling_ok_low(void) {
    OmobdOilMonitor m; omobdOilMonitorReset(m);
    OmobdOilConfig c = cfg();
    // Key on, engine stopped: zero oil pressure is normal, not an alarm.
    TEST_ASSERT_EQUAL(OMOBD_OIL_ENGINE_STOPPED, omobdOilMonitorUpdate(m, c, 0.0f, 0.0f, 1000));
    // Start: warmup window ignores low pressure.
    TEST_ASSERT_EQUAL(OMOBD_OIL_SETTLING, omobdOilMonitorUpdate(m, c, 750.0f, 0.1f, 2000));
    TEST_ASSERT_EQUAL(OMOBD_OIL_SETTLING, omobdOilMonitorUpdate(m, c, 800.0f, 0.2f, 6900));
    // After warmup with good pressure.
    TEST_ASSERT_EQUAL(OMOBD_OIL_OK, omobdOilMonitorUpdate(m, c, 800.0f, 2.5f, 7100));
    // Pressure drops: pending until debounce elapses.
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW_PENDING, omobdOilMonitorUpdate(m, c, 2000.0f, 1.0f, 8000));
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW_PENDING, omobdOilMonitorUpdate(m, c, 2000.0f, 1.0f, 9900));
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW, omobdOilMonitorUpdate(m, c, 2000.0f, 1.0f, 10000));
    // Recovery clears immediately.
    TEST_ASSERT_EQUAL(OMOBD_OIL_OK, omobdOilMonitorUpdate(m, c, 2000.0f, 3.0f, 10100));
}

static void test_oil_brief_dip_is_not_low(void) {
    OmobdOilMonitor m; omobdOilMonitorReset(m);
    OmobdOilConfig c = cfg();
    omobdOilMonitorUpdate(m, c, 800.0f, 2.0f, 1);
    omobdOilMonitorUpdate(m, c, 800.0f, 2.0f, 6000);
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW_PENDING, omobdOilMonitorUpdate(m, c, 800.0f, 0.1f, 6100));
    TEST_ASSERT_EQUAL(OMOBD_OIL_OK, omobdOilMonitorUpdate(m, c, 800.0f, 2.0f, 6500));
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW_PENDING, omobdOilMonitorUpdate(m, c, 800.0f, 0.1f, 6600));
    // debounce restarted at 6600, so 8500 is still pending
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW_PENDING, omobdOilMonitorUpdate(m, c, 800.0f, 0.1f, 8500));
}

static void test_oil_sensor_faults(void) {
    OmobdOilMonitor m; omobdOilMonitorReset(m);
    OmobdOilConfig c = cfg();
    TEST_ASSERT_EQUAL(OMOBD_OIL_SENSOR_FAULT, omobdOilMonitorUpdate(m, c, NAN, 2.0f, 1000));
    omobdOilMonitorUpdate(m, c, 900.0f, 2.0f, 2000);
    TEST_ASSERT_EQUAL(OMOBD_OIL_SENSOR_FAULT, omobdOilMonitorUpdate(m, c, 900.0f, NAN, 8000));
}

static void test_oil_millis_zero_start(void) {
    OmobdOilMonitor m; omobdOilMonitorReset(m);
    OmobdOilConfig c{CURVE, 3, 300.0f, 0, 0};
    // now == 0 must not be mistaken for "not started"
    TEST_ASSERT_EQUAL(OMOBD_OIL_LOW, omobdOilMonitorUpdate(m, c, 1000.0f, 0.0f, 0));
}

// ---- engine hours ---------------------------------------------------------

static void test_hours_accumulate_only_when_running(void) {
    OmobdHourMeter h; omobdHourMeterInit(h, 0);
    omobdHourMeterUpdate(h, 0.0f, 300.0f, 1000, 2000);
    omobdHourMeterUpdate(h, 0.0f, 300.0f, 2000, 2000);
    TEST_ASSERT_EQUAL_UINT64(0, h.runMs);
    omobdHourMeterUpdate(h, 800.0f, 300.0f, 3000, 2000);
    omobdHourMeterUpdate(h, 800.0f, 300.0f, 4000, 2000);
    TEST_ASSERT_EQUAL_UINT64(2000, h.runMs);
    omobdHourMeterUpdate(h, NAN, 300.0f, 5000, 2000);     // no RPM: no invented time
    TEST_ASSERT_EQUAL_UINT64(2000, h.runMs);
}

static void test_hours_skip_stalls_and_resume_from_storage(void) {
    OmobdHourMeter h; omobdHourMeterInit(h, 3600000ULL * 100);   // 100 h stored
    omobdHourMeterUpdate(h, 800.0f, 300.0f, 1000, 2000);
    omobdHourMeterUpdate(h, 800.0f, 300.0f, 60000, 2000);       // 59 s gap = stall, not counted
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 100.0f, omobdHourMeterHours(h));
    omobdHourMeterUpdate(h, 800.0f, 300.0f, 61000, 2000);
    TEST_ASSERT_EQUAL_UINT64(3600000ULL * 100 + 1000, h.runMs);
}

static void test_hours_persist_cadence(void) {
    OmobdHourMeter h; omobdHourMeterInit(h, 0);
    uint32_t t = 1;
    omobdHourMeterUpdate(h, 900.0f, 300.0f, t, 2000);
    for (int i = 0; i < 360; i++) { t += 1000; omobdHourMeterUpdate(h, 900.0f, 300.0f, t, 2000); }
    TEST_ASSERT_TRUE(omobdHourMeterShouldPersist(h, 360000));
    omobdHourMeterMarkPersisted(h);
    TEST_ASSERT_FALSE(omobdHourMeterShouldPersist(h, 360000));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gauge_from_absolute);
    RUN_TEST(test_vacuum_from_absolute);
    RUN_TEST(test_pressure_nan_and_bad_reference);
    RUN_TEST(test_altitude_reference_changes_boost);
    RUN_TEST(test_unit_conversions);
    RUN_TEST(test_health_states);
    RUN_TEST(test_health_unknown_range_only_flags_nan);
    RUN_TEST(test_health_names);
    RUN_TEST(test_oil_curve_interpolation);
    RUN_TEST(test_oil_curve_validation);
    RUN_TEST(test_oil_unconfigured_judges_nothing);
    RUN_TEST(test_oil_stopped_settling_ok_low);
    RUN_TEST(test_oil_brief_dip_is_not_low);
    RUN_TEST(test_oil_sensor_faults);
    RUN_TEST(test_oil_millis_zero_start);
    RUN_TEST(test_hours_accumulate_only_when_running);
    RUN_TEST(test_hours_skip_stalls_and_resume_from_storage);
    RUN_TEST(test_hours_persist_cadence);
    return UNITY_END();
}
