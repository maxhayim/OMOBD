/*
 * omobd_live.cpp - Live-data JSON for the OMOBD web dashboard. See omobd_live.h.
 */
// Compiled only into OMOBD builds and host tests; upstream preOBD boards
// get byte-identical firmware.
#if (defined(OMOBD_WEB_DASHBOARD) && OMOBD_WEB_DASHBOARD) || defined(NATIVE_TEST)

#include "omobd_live.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

bool omobdJsonEscape(const char* in, char* out, size_t cap) {
    if (!out || cap == 0) return false;
    size_t o = 0;
    if (!in) in = "";
    for (const unsigned char* p = (const unsigned char*)in; *p; p++) {
        char tmp[7];
        const char* rep = nullptr;
        switch (*p) {
            case '"':  rep = "\\\""; break;
            case '\\': rep = "\\\\"; break;
            case '\n': rep = "\\n"; break;
            case '\r': rep = "\\r"; break;
            case '\t': rep = "\\t"; break;
            default:
                if (*p < 0x20) { snprintf(tmp, sizeof(tmp), "\\u%04x", *p); rep = tmp; }
        }
        if (rep) {
            size_t l = strlen(rep);
            if (o + l >= cap) { out[0] = '\0'; return false; }
            memcpy(out + o, rep, l);
            o += l;
        } else {
            if (o + 1 >= cap) { out[0] = '\0'; return false; }
            out[o++] = (char)*p;
        }
    }
    out[o] = '\0';
    return true;
}

namespace {
struct Writer {
    char* buf; size_t cap; size_t len; bool ok;
    void put(const char* fmt, ...) {
        if (!ok) return;
        va_list ap;
        va_start(ap, fmt);
        int n = vsnprintf(buf + len, cap - len, fmt, ap);
        va_end(ap);
        if (n < 0 || (size_t)n >= cap - len) { ok = false; return; }
        len += (size_t)n;
    }
    void str(const char* key, const char* v) {
        char esc[96];
        if (!omobdJsonEscape(v ? v : "", esc, sizeof(esc))) { ok = false; return; }
        put("\"%s\":\"%s\"", key, esc);
    }
    void num(const char* key, float v, int decimals) {
        if (!isfinite(v)) put("\"%s\":null", key);
        else put("\"%s\":%.*f", key, decimals, (double)v);
    }
};
}  // namespace

size_t omobdWriteLiveJson(char* out, size_t cap, const OmobdLiveMeta& meta,
                          const OmobdLiveChannel* ch, uint8_t count,
                          const OmobdLiveDerived& d) {
    if (!out || cap < 2) return 0;
    Writer w{out, cap, 0, true};
    w.put("{\"seq\":%lu,\"uptimeMs\":%lu,", (unsigned long)meta.seq, (unsigned long)meta.uptimeMs);
    w.str("mode", meta.mode); w.put(",");
    w.put("\"systemSeverity\":%u,", (unsigned)meta.systemSeverity);
    w.str("profile", meta.profileId); w.put(",");
    w.str("firmware", meta.firmware); w.put(",\"channels\":[");
    for (uint8_t i = 0; i < count; i++) {
        const OmobdLiveChannel& c = ch[i];
        bool valid = c.health == OMOBD_HEALTH_OK || c.health == OMOBD_HEALTH_OUT_OF_RANGE;
        if (i) w.put(",");
        w.put("{\"pin\":%u,", (unsigned)c.pin);
        w.str("key", c.key); w.put(",");
        w.str("abbr", c.abbr); w.put(",");
        w.str("label", c.label); w.put(",");
        w.str("unit", c.unit); w.put(",");
        w.num("value", valid ? c.display : NAN, 2); w.put(",");
        w.num("base", valid ? c.base : NAN, 3); w.put(",");
        w.str("health", omobdHealthName(c.health)); w.put(",");
        w.str("src", omobdSourceName(c.source)); w.put(",");
        w.put("\"severity\":%u}", (unsigned)c.severity);
    }
    w.put("],\"derived\":{");
    w.put("\"atmosphere\":{"); w.num("bar", d.atmBar, 4); w.put(",");
    w.str("source", d.atmSource); w.put(",\"src\":\"%s\"},", omobdSourceName(OMOBD_SRC_CALCULATED));
    w.put("\"boost\":{"); w.num("bar", d.boostBar, 3); w.put(",\"src\":\"calculated\"},");
    w.put("\"vacuum\":{"); w.num("bar", d.vacuumBar, 3); w.put(",\"src\":\"calculated\"},");
    w.put("\"engineHours\":{"); w.num("hours", d.engineHours, 2); w.put(",\"src\":\"calculated\"},");
    w.put("\"oilVsRpm\":{"); w.str("state", d.oilState); w.put(",");
    w.num("minBar", d.oilMinBar, 2); w.put(",\"src\":\"calculated\"}");
    w.put("}}");
    if (!w.ok) { out[0] = '\0'; return 0; }
    return w.len;
}

#endif // OMOBD build guard
