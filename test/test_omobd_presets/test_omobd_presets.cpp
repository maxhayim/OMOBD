// OMOBD native tests: the two application presets OMOBD adds to preOBD
// (VACUUM, ENGINE_BAY_TEMP) and their OBD-style PIDs 0xB8/0xB9.
#include <unity.h>
#include <Arduino.h>
#include <string.h>

#include "../../src/sensors/sensor_library.cpp"
#include "../../src/sensors/sensor_library/sensor_categories.cpp"
#include "../../src/sensors/units_registry.cpp"
#include "../../src/sensors/application_presets.cpp"
#include "../../src/inputs/measurement_compat.cpp"
#include "../../src/sensors/can_sensor_library/standard_pids.cpp"
#include "../../src/sensors/can_sensor_library/obd_output_pids.cpp"
#include "../../src/outputs/obd_encode.cpp"

void setUp(void) {}
void tearDown(void) {}

static void test_existing_indices_unchanged(void) {
    // Stored configs hold application indices; OMOBD only appends.
    TEST_ASSERT_EQUAL(28, getApplicationIndexByName("INTERCOOLER_TEMP"));
    TEST_ASSERT_EQUAL(2, getApplicationIndexByName("EGT"));
    TEST_ASSERT_EQUAL(29, getApplicationIndexByName("VACUUM"));
    TEST_ASSERT_EQUAL(30, getApplicationIndexByName("ENGINE_BAY_TEMP"));
}

static void test_presets_have_no_default_alarm(void) {
    // No universal vacuum or under-hood limits are asserted for OM engines.
    ApplicationPreset p;
    loadApplicationPreset(getApplicationByIndex(29), &p);
    TEST_ASSERT_FALSE(p.defaultAlarmEnabled);
    TEST_ASSERT_EQUAL(MEASURE_PRESSURE, p.expectedMeasurementType);
    TEST_ASSERT_EQUAL_HEX8(0xB8, p.obd2pid);
    loadApplicationPreset(getApplicationByIndex(30), &p);
    TEST_ASSERT_FALSE(p.defaultAlarmEnabled);
    TEST_ASSERT_EQUAL(MEASURE_TEMPERATURE, p.expectedMeasurementType);
    TEST_ASSERT_EQUAL_HEX8(0xB9, p.obd2pid);
}

static void test_vacuum_pid_encoding(void) {
    ObdOutputPid s;
    TEST_ASSERT_TRUE(obdOutputSpec(0xB8, s));
    uint8_t payload[OBD_OUT_MAX_LENGTH];
    // 0.30 bar absolute = 30.0 kPa = raw 300 = 0x012C
    TEST_ASSERT_EQUAL(2, obdEncodeValue(s, 0.30f, false, 1.013f, payload));
    TEST_ASSERT_EQUAL_HEX8(0x01, payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0x2C, payload[1]);
    // A missing reading is never encoded as a value.
    TEST_ASSERT_EQUAL(0, obdEncodeValue(s, NAN, false, 1.013f, payload));
}

static void test_engine_bay_pid_encoding(void) {
    ObdOutputPid s;
    TEST_ASSERT_TRUE(obdOutputSpec(0xB9, s));
    uint8_t payload[OBD_OUT_MAX_LENGTH];
    // 60 C -> (60+40)*10 = 1000 = 0x03E8
    TEST_ASSERT_EQUAL(2, obdEncodeValue(s, 60.0f, false, 1.013f, payload));
    TEST_ASSERT_EQUAL_HEX8(0x03, payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0xE8, payload[1]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_existing_indices_unchanged);
    RUN_TEST(test_presets_have_no_default_alarm);
    RUN_TEST(test_vacuum_pid_encoding);
    RUN_TEST(test_engine_bay_pid_encoding);
    return UNITY_END();
}
