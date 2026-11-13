/*
 * omobd_derived.cpp - OMOBD calculated values and input health (pure logic)
 * See omobd_derived.h.
 */
// Compiled only into OMOBD builds and host tests; upstream preOBD boards
// get byte-identical firmware.
#if (defined(OMOBD_WEB_DASHBOARD) && OMOBD_WEB_DASHBOARD) || defined(NATIVE_TEST)

#include "omobd_derived.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Pressure
// ---------------------------------------------------------------------------

bool omobdAtmValid(float atmBar) {
    return isfinite(atmBar) && atmBar >= OMOBD_ATM_MIN_BAR && atmBar <= OMOBD_ATM_MAX_BAR;
}

float omobdGaugeFromAbsolute(float absBar, float atmBar) {
    if (!isfinite(absBar) || !omobdAtmValid(atmBar)) return NAN;
    return absBar - atmBar;
}

float omobdVacuumFromAbsolute(float absBar, float atmBar) {
    if (!isfinite(absBar) || !omobdAtmValid(atmBar)) return NAN;
    return atmBar - absBar;
}

float omobdBarToPsi(float bar)  { return bar * 14.5037738f; }
float omobdBarToKpa(float bar)  { return bar * 100.0f; }
float omobdBarToInHg(float bar) { return bar * 29.5299831f; }

// ---------------------------------------------------------------------------
// Health
// ---------------------------------------------------------------------------

const char* omobdHealthName(OmobdHealth h) {
    switch (h) {
        case OMOBD_HEALTH_OK:           return "ok";
        case OMOBD_HEALTH_NO_READING:   return "no_reading";
        case OMOBD_HEALTH_FAULT:        return "fault";
        case OMOBD_HEALTH_OUT_OF_RANGE: return "out_of_range";
        case OMOBD_HEALTH_DISABLED:     return "disabled";
    }
    return "unknown";
}

OmobdHealth omobdClassifyInput(bool enabled, bool hasReading, float value,
                               float sensorMin, float sensorMax, float marginFrac) {
    if (!enabled) return OMOBD_HEALTH_DISABLED;
    if (!hasReading) return OMOBD_HEALTH_NO_READING;
    if (!isfinite(value)) return OMOBD_HEALTH_FAULT;
    if (isfinite(sensorMin) && isfinite(sensorMax) && sensorMin < sensorMax) {
        if (!(marginFrac >= 0.0f)) marginFrac = 0.0f;
        float m = (sensorMax - sensorMin) * marginFrac;
        if (value < sensorMin - m || value > sensorMax + m) return OMOBD_HEALTH_OUT_OF_RANGE;
    }
    return OMOBD_HEALTH_OK;
}

const char* omobdSourceName(OmobdSource s) {
    switch (s) {
        case OMOBD_SRC_MEASURED:   return "measured";
        case OMOBD_SRC_CALCULATED: return "calculated";
        case OMOBD_SRC_IMPORTED:   return "imported";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Oil pressure vs RPM
// ---------------------------------------------------------------------------

bool omobdOilCurveValid(const OmobdOilPoint* curve, uint8_t count) {
    if (!curve || count == 0 || count > OMOBD_OIL_CURVE_MAX) return false;
    for (uint8_t i = 0; i < count; i++) {
        if (!isfinite(curve[i].rpm) || !isfinite(curve[i].minBar)) return false;
        if (curve[i].rpm < 0.0f) return false;
        if (curve[i].minBar < 0.0f || curve[i].minBar > 10.0f) return false;
        if (i > 0 && !(curve[i].rpm > curve[i - 1].rpm)) return false;
    }
    return true;
}

float omobdOilMinForRpm(const OmobdOilPoint* curve, uint8_t count, float rpm) {
    if (!curve || count == 0 || !isfinite(rpm)) return NAN;
    if (rpm <= curve[0].rpm) return curve[0].minBar;
    if (rpm >= curve[count - 1].rpm) return curve[count - 1].minBar;
    for (uint8_t i = 1; i < count; i++) {
        if (rpm <= curve[i].rpm) {
            float span = curve[i].rpm - curve[i - 1].rpm;
            float t = (rpm - curve[i - 1].rpm) / span;
            return curve[i - 1].minBar + t * (curve[i].minBar - curve[i - 1].minBar);
        }
    }
    return curve[count - 1].minBar;
}

const char* omobdOilStateName(OmobdOilState s) {
    switch (s) {
        case OMOBD_OIL_UNCONFIGURED:   return "unconfigured";
        case OMOBD_OIL_ENGINE_STOPPED: return "engine_stopped";
        case OMOBD_OIL_SETTLING:       return "settling";
        case OMOBD_OIL_OK:             return "ok";
        case OMOBD_OIL_LOW_PENDING:    return "low_pending";
        case OMOBD_OIL_LOW:            return "low";
        case OMOBD_OIL_SENSOR_FAULT:   return "sensor_fault";
    }
    return "unknown";
}

void omobdOilMonitorReset(OmobdOilMonitor& m) {
    m.state = OMOBD_OIL_UNCONFIGURED;
    m.runningSince = 0;
    m.lowSince = 0;
}

// millis() can legitimately be 0 at boot; store 1 instead so 0 keeps meaning "unset".
static inline uint32_t stamp(uint32_t now) { return now ? now : 1; }

OmobdOilState omobdOilMonitorUpdate(OmobdOilMonitor& m, const OmobdOilConfig& cfg,
                                    float rpm, float oilBar, uint32_t now) {
    if (!omobdOilCurveValid(cfg.curve, cfg.count) || !isfinite(cfg.runningRpm)) {
        omobdOilMonitorReset(m);
        return m.state;
    }
    if (!isfinite(rpm)) {
        // Without RPM we can't know which limit applies — report, don't guess.
        m.lowSince = 0;
        m.state = OMOBD_OIL_SENSOR_FAULT;
        return m.state;
    }
    if (rpm < cfg.runningRpm) {
        m.runningSince = 0;
        m.lowSince = 0;
        m.state = OMOBD_OIL_ENGINE_STOPPED;
        return m.state;
    }
    if (m.runningSince == 0) m.runningSince = stamp(now);
    if ((uint32_t)(now - m.runningSince) < cfg.warmupMs) {
        m.lowSince = 0;
        m.state = OMOBD_OIL_SETTLING;
        return m.state;
    }
    if (!isfinite(oilBar)) {
        m.lowSince = 0;
        m.state = OMOBD_OIL_SENSOR_FAULT;
        return m.state;
    }
    float minBar = omobdOilMinForRpm(cfg.curve, cfg.count, rpm);
    if (oilBar >= minBar) {
        m.lowSince = 0;
        m.state = OMOBD_OIL_OK;
        return m.state;
    }
    if (m.lowSince == 0) m.lowSince = stamp(now);
    m.state = ((uint32_t)(now - m.lowSince) >= cfg.debounceMs) ? OMOBD_OIL_LOW
                                                               : OMOBD_OIL_LOW_PENDING;
    return m.state;
}

// ---------------------------------------------------------------------------
// Engine hours
// ---------------------------------------------------------------------------

void omobdHourMeterInit(OmobdHourMeter& h, uint64_t storedRunMs) {
    h.runMs = storedRunMs;
    h.lastUpdate = 0;
    h.unsavedMs = 0;
}

void omobdHourMeterUpdate(OmobdHourMeter& h, float rpm, float runningRpm,
                          uint32_t now, uint32_t maxGapMs) {
    uint32_t prev = h.lastUpdate;
    h.lastUpdate = stamp(now);
    if (prev == 0) return;                       // first sample: nothing elapsed yet
    if (!isfinite(rpm) || !isfinite(runningRpm)) return;
    if (rpm < runningRpm) return;
    uint32_t dt = now - prev;
    if (dt > maxGapMs) return;                   // loop stalled: don't credit unknown time
    h.runMs += dt;
    h.unsavedMs += dt;
}

float omobdHourMeterHours(const OmobdHourMeter& h) {
    return (float)((double)h.runMs / 3600000.0);
}

bool omobdHourMeterShouldPersist(const OmobdHourMeter& h, uint32_t persistEveryMs) {
    return h.unsavedMs >= persistEveryMs;
}

void omobdHourMeterMarkPersisted(OmobdHourMeter& h) { h.unsavedMs = 0; }

#endif // OMOBD build guard
