/*
 * omobd_live.h - Live-data JSON for the OMOBD web dashboard (pure logic)
 *
 * The firmware glue (omobd_web.cpp) copies preOBD's inputs[] into LiveChannel
 * records; this module turns them into the /api/live document. Writing into a
 * caller-owned buffer with no heap keeps RUN-mode memory flat.
 *
 * Rules the document follows:
 *   - A value is a number only when it is a finite reading. A fault, an input
 *     that has never been read, or a disabled input is `null` — never 0.
 *   - Every channel carries `src` (measured / calculated / imported) so the
 *     dashboard can label calculated values as such.
 */
#ifndef OMOBD_LIVE_H
#define OMOBD_LIVE_H

#include <stdint.h>
#include <stddef.h>
#include "omobd_derived.h"

struct OmobdLiveChannel {
    uint8_t     pin;
    const char* key;        // stable id for layouts: preOBD application name, e.g. "EGT"
    const char* abbr;       // short label
    const char* label;      // long label
    const char* unit;       // display unit symbol
    float       display;    // value in display units (NaN when not valid)
    float       base;       // value in preOBD base units (C, bar, V, rpm ...)
    OmobdHealth health;
    OmobdSource source;
    uint8_t     severity;   // 0 none, 1 info, 2 warning, 3 alarm (preOBD RuleSeverity)
};

struct OmobdLiveDerived {
    float    atmBar;        // reference in use (NaN = none)
    const char* atmSource;  // "FIXED" / "INPUT" / "KEY_ON_MAP" / "none"
    float    boostBar;      // gauge, NaN when not fitted/invalid
    float    vacuumBar;     // below-atmosphere depth, NaN when not fitted/invalid
    float    engineHours;   // NaN when no RPM input
    const char* oilState;   // omobdOilStateName()
    float    oilMinBar;     // current curve limit (NaN when unconfigured)
};

struct OmobdLiveMeta {
    uint32_t    uptimeMs;
    const char* mode;       // "RUN" / "CONFIG"
    uint8_t     systemSeverity;
    const char* profileId;
    const char* firmware;
    uint32_t    seq;        // increments per document, lets the client spot stale data
};

// Returns bytes written (excluding NUL), or 0 if `cap` was too small — the
// caller must then send an error rather than a truncated document.
size_t omobdWriteLiveJson(char* out, size_t cap, const OmobdLiveMeta& meta,
                          const OmobdLiveChannel* ch, uint8_t count,
                          const OmobdLiveDerived& d);

// JSON-escape `in` into `out`. Returns false if it didn't fit.
bool omobdJsonEscape(const char* in, char* out, size_t cap);

#endif // OMOBD_LIVE_H
