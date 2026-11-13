/*
 * omobd_derived.h - OMOBD calculated values and input health (pure logic)
 *
 * Part of OMOBD, an adaptation of preOBD for Mercedes-Benz OM diesel engines.
 * Everything here is host-testable: no Arduino, no globals, no I/O.
 *
 * What lives here, and why it is separate from preOBD's inputs:
 *   - Boost (gauge) and vacuum are CALCULATED from an absolute MAP reading and
 *     an atmospheric reference. They are never presented as direct measurements.
 *   - Input health classification (no reading yet / sensor fault / reading
 *     outside the sensor's own range) used by the web dashboard.
 *   - RPM-dependent minimum oil pressure monitor (advisory only; it switches
 *     nothing on the vehicle).
 *   - Engine-hours accumulator driven by a measured RPM signal.
 *
 * Units: pressure in bar (absolute or gauge as named), RPM in rev/min,
 * time in milliseconds — the same base units preOBD uses.
 */
#ifndef OMOBD_DERIVED_H
#define OMOBD_DERIVED_H

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Pressure helpers
// ---------------------------------------------------------------------------

// Standard atmosphere at sea level, bar. Used only as the DEFAULT fixed
// reference; users at altitude should set their own or wire a barometer.
static constexpr float OMOBD_STD_ATM_BAR = 1.01325f;

// Plausible atmospheric reference band (bar absolute). Covers sea level
// weather extremes down to roughly 5,000 m elevation.
static constexpr float OMOBD_ATM_MIN_BAR = 0.50f;
static constexpr float OMOBD_ATM_MAX_BAR = 1.10f;

bool  omobdAtmValid(float atmBar);

// Gauge pressure = absolute - atmospheric. Positive = boost, negative = below
// atmosphere. NaN in -> NaN out; an invalid reference -> NaN.
float omobdGaugeFromAbsolute(float absBar, float atmBar);

// Vacuum depth = atmospheric - absolute (positive when below atmosphere).
// Returned in bar; convert with omobdBarToInHg() for the usual vacuum unit.
float omobdVacuumFromAbsolute(float absBar, float atmBar);

float omobdBarToPsi(float bar);
float omobdBarToKpa(float bar);
float omobdBarToInHg(float bar);

// ---------------------------------------------------------------------------
// Input health
// ---------------------------------------------------------------------------

enum OmobdHealth : uint8_t {
    OMOBD_HEALTH_OK = 0,        // finite reading inside the sensor's range
    OMOBD_HEALTH_NO_READING,    // enabled but never read since configured/boot
    OMOBD_HEALTH_FAULT,         // driver reported NaN: open/short/bus error
    OMOBD_HEALTH_OUT_OF_RANGE,  // finite, but outside what this sensor can report
    OMOBD_HEALTH_DISABLED,      // input switched off
};

const char* omobdHealthName(OmobdHealth h);

// sensorMin/sensorMax are the sensor library's reportable range in base units.
// If sensorMin >= sensorMax (range unknown) only NaN is treated as a fault.
// marginFrac widens the band (0.05 = 5 % of span each side) so normal ADC
// noise at the rails is not flagged.
OmobdHealth omobdClassifyInput(bool enabled, bool hasReading, float value,
                               float sensorMin, float sensorMax, float marginFrac);

// Where a value comes from. Shown on every dashboard tile.
enum OmobdSource : uint8_t {
    OMOBD_SRC_MEASURED = 0,    // a sensor wired to this device
    OMOBD_SRC_CALCULATED,      // derived from measured values (boost, vacuum, hours)
    OMOBD_SRC_IMPORTED,        // read from another controller over CAN
};

const char* omobdSourceName(OmobdSource s);

// ---------------------------------------------------------------------------
// RPM-dependent minimum oil pressure (advisory)
// ---------------------------------------------------------------------------

static constexpr uint8_t OMOBD_OIL_CURVE_MAX = 6;

struct OmobdOilPoint {
    float rpm;      // engine speed, ascending across the curve
    float minBar;   // minimum acceptable oil pressure (gauge) at that speed
};

// Minimum pressure for `rpm`, linearly interpolated between points and held
// flat beyond either end. Returns NaN when the curve is empty or rpm is NaN.
float omobdOilMinForRpm(const OmobdOilPoint* curve, uint8_t count, float rpm);

// True when points are strictly ascending in rpm, rpm >= 0 and 0 <= minBar <= 10.
bool omobdOilCurveValid(const OmobdOilPoint* curve, uint8_t count);

enum OmobdOilState : uint8_t {
    OMOBD_OIL_UNCONFIGURED = 0,  // no validated curve: nothing is judged
    OMOBD_OIL_ENGINE_STOPPED,    // RPM below running threshold
    OMOBD_OIL_SETTLING,          // engine just started: warmup window
    OMOBD_OIL_OK,
    OMOBD_OIL_LOW_PENDING,       // below curve, debounce running
    OMOBD_OIL_LOW,               // below curve for longer than debounce
    OMOBD_OIL_SENSOR_FAULT,      // RPM or pressure missing/invalid while running
};

const char* omobdOilStateName(OmobdOilState s);

struct OmobdOilMonitor {
    OmobdOilState state;
    uint32_t runningSince;   // millis() when RPM first crossed the threshold (0 = stopped)
    uint32_t lowSince;       // millis() when the low condition started (0 = not low)
};

struct OmobdOilConfig {
    const OmobdOilPoint* curve;
    uint8_t  count;
    float    runningRpm;     // engine counts as running at/above this
    uint16_t warmupMs;       // ignore readings this long after start
    uint16_t debounceMs;     // low must persist this long before LOW
};

void omobdOilMonitorReset(OmobdOilMonitor& m);
// Advance the monitor. rpm/oilBar may be NaN (sensor fault). Returns the new state.
OmobdOilState omobdOilMonitorUpdate(OmobdOilMonitor& m, const OmobdOilConfig& cfg,
                                    float rpm, float oilBar, uint32_t now);

// ---------------------------------------------------------------------------
// Engine hours
// ---------------------------------------------------------------------------

struct OmobdHourMeter {
    uint64_t runMs;          // accumulated running time
    uint32_t lastUpdate;     // millis() of the previous update (0 = first call)
    uint32_t unsavedMs;      // running time not yet persisted
};

void  omobdHourMeterInit(OmobdHourMeter& h, uint64_t storedRunMs);
// Accumulates elapsed time while rpm >= runningRpm. A NaN RPM accumulates
// nothing (we never invent running time). Gaps longer than maxGapMs (a stall
// in the main loop) are not counted.
void  omobdHourMeterUpdate(OmobdHourMeter& h, float rpm, float runningRpm,
                           uint32_t now, uint32_t maxGapMs);
float omobdHourMeterHours(const OmobdHourMeter& h);
// True when enough unsaved time has built up to be worth a flash write.
bool  omobdHourMeterShouldPersist(const OmobdHourMeter& h, uint32_t persistEveryMs);
void  omobdHourMeterMarkPersisted(OmobdHourMeter& h);

#endif // OMOBD_DERIVED_H
