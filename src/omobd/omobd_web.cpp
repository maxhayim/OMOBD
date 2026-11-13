/*
 * omobd_web.cpp - OMOBD Wi-Fi web dashboard (ESP32-S3). See omobd_web.h.
 *
 * Design notes (reliability was the weak point of the original EDMS-II):
 *   - Runs in loop() on the same task as sensor reads: no locking, no races
 *     with inputs[]. Every handler is short; the one long response (config
 *     export) is streamed in chunks and feeds the watchdog.
 *   - All buffers are static and allocated at boot. RUN mode does not grow
 *     the heap.
 *   - Wi-Fi is supervised: a dropped station link falls back to the
 *     device's own access point, and a failed AP start is retried.
 *   - The dashboard polls /api/live. If the phone sleeps or walks out of
 *     range, nothing on the device waits for it.
 *   - Settings are validated on every load and every write; a bad or
 *     corrupt store falls back to defaults and says so in /api/status.
 */
#if defined(OMOBD_WEB_DASHBOARD) && OMOBD_WEB_DASHBOARD

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_system.h>
#include <math.h>

#include "omobd_web.h"
#include "omobd_settings.h"
#include "omobd_live.h"
#include "omobd_history.h"
#include "omobd_derived.h"
#include "dashboard_html.h"

#include "../config.h"
#include "../version.h"
#include "../inputs/input.h"
#include "../inputs/input_manager.h"
#include "../sensors/sensor_library.h"
#include "../sensors/application_presets.h"
#include "../sensors/units_registry.h"
#include "../core/system_mode.h"
#include "../core/watchdog.h"
#include "../core/message_api.h"
#include "../core/log_tags.h"
#include "../storage/json_config.h"
#include "../outputs/output_base.h"
#include "../rules/rules_manager.h"

namespace {

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

WebServer   server(80);
Preferences prefs;

OmobdSettings settings;
char     settingsNote[48] = "";      // why defaults were used, if they were
bool     settingsFromStore = false;

OmobdOilMonitor oilMon;
OmobdHourMeter  hourMeter;
bool            hourMeterActive = false;

float    keyOnAtmBar = NAN;          // KEY_ON_MAP capture
uint32_t lastLiveSeq = 0;

// History: one ring per tracked input pin plus boost and vacuum.
constexpr size_t   HIST_LEN = 600;    // 10 min at the default 1 s interval
constexpr uint8_t  HIST_SLOTS = 12;
struct HistSlot { uint8_t pin; OmobdHistoryRing<HIST_LEN> ring; };
HistSlot histSlots[HIST_SLOTS];
OmobdHistoryRing<HIST_LEN> histBoost, histVacuum;
uint32_t lastHistSample = 0;

// Wi-Fi supervision
enum class Net : uint8_t { OFF, AP, STA_CONNECTING, STA, STA_AND_AP };
Net      net = Net::OFF;
uint32_t netStateSince = 0;
uint32_t lastApRetry = 0;
constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t AP_RETRY_MS = 30000;

char liveBuf[10240];
char smallBuf[2048];

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

Input* inputForPin(uint8_t pin) {
    if (pin == OMOBD_PIN_NONE) return nullptr;
    for (uint8_t i = 0; i < MAX_INPUTS; i++) {
        if (inputs[i].pin == pin) return &inputs[i];
    }
    return nullptr;
}

// Value of an input in base units, or NaN unless it is a trustworthy reading.
float baseValue(uint8_t pin) {
    Input* in = inputForPin(pin);
    if (!in || !in->flags.isEnabled || !in->flags.hasReading) return NAN;
    return in->value;
}

float atmosphereBar(const char** srcName) {
    switch (settings.atmSource) {
        case OMOBD_ATM_INPUT: {
            float v = baseValue(settings.atmInputPin);
            // BAROMETRIC_PRESSURE inputs report in bar (preOBD base unit).
            if (omobdAtmValid(v)) { *srcName = "INPUT"; return v; }
            break;  // fall back to the fixed value rather than show nothing
        }
        case OMOBD_ATM_KEY_ON_MAP:
            if (omobdAtmValid(keyOnAtmBar)) { *srcName = "KEY_ON_MAP"; return keyOnAtmBar; }
            break;
        default:
            break;
    }
    *srcName = "FIXED";
    return settings.atmFixedBar;
}

// Capture the boost MAP sensor as the atmospheric reference — only while the
// engine is verifiably stopped (RPM input present and below running speed).
void maybeCaptureKeyOnAtm() {
    if (settings.atmSource != OMOBD_ATM_KEY_ON_MAP || isfinite(keyOnAtmBar)) return;
    float rpm = baseValue(settings.rpmPin);
    float map = baseValue(settings.boostMapPin);
    if (!isfinite(rpm) || rpm >= settings.runningRpm) return;
    if (omobdAtmValid(map)) {
        keyOnAtmBar = map;
        msg.debug.info(TAG_SYSTEM, "OMOBD: key-on atmospheric reference captured");
    }
}

uint8_t severityForInput(const Input& in) {
#if INCLUDE_RULES_ENGINE
    uint8_t worst = RULE_SEV_NONE;
    for (int i = 0; i < MAX_RULES; i++) {
        if (rules[i].id == RULES_ID_NONE || rules[i].sourceInput != in.pin) continue;
        uint8_t s = rulesRuleAnnunciatorSeverity(rules[i], g_ruleContexts[i], in.value);
        if (s > worst) worst = s;
    }
    return worst;
#else
    (void)in;
    return 0;
#endif
}

OmobdSource sourceForInput(const Input& in) {
    if (in.calibrationType == CAL_CAN_IMPORT) return OMOBD_SRC_IMPORTED;
    const ApplicationPreset* app = getApplicationByIndex(in.applicationIndex);
    const char* name = app ? READ_APP_NAME(app) : nullptr;
    // BME280 elevation is computed from pressure, not measured.
    if (name && strcmp(name, "ELEVATION") == 0) return OMOBD_SRC_CALCULATED;
    return OMOBD_SRC_MEASURED;
}

void sampleHistory(uint32_t now, float boost, float vacuum) {
    if (now - lastHistSample < settings.historyIntervalMs) return;
    lastHistSample = now;
    // Free slots whose input has gone, then sample every enabled input.
    for (auto& s : histSlots) {
        if (s.pin != OMOBD_PIN_NONE && !inputForPin(s.pin)) { s.pin = OMOBD_PIN_NONE; s.ring.clear(); }
    }
    for (uint8_t i = 0; i < MAX_INPUTS; i++) {
        const Input& in = inputs[i];
        if (in.pin == 0xFF || !in.flags.isEnabled) continue;
        HistSlot* slot = nullptr;
        for (auto& s : histSlots) if (s.pin == in.pin) { slot = &s; break; }
        if (!slot) for (auto& s : histSlots) if (s.pin == OMOBD_PIN_NONE) { slot = &s; s.pin = in.pin; break; }
        if (!slot) continue;  // more inputs than slots: the newest go untracked
        slot->ring.push(in.flags.hasReading ? in.value : NAN);
    }
    histBoost.push(boost);
    histVacuum.push(vacuum);
}

// ---------------------------------------------------------------------------
// Persistence (ESP32 NVS, namespace "omobd")
// ---------------------------------------------------------------------------

void generateApPassword(char* out, size_t cap) {
    static const char alphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";  // no 0/O/1/l/i
    size_t n = 12;
    if (cap < n + 1) n = cap - 1;
    for (size_t i = 0; i < n; i++) out[i] = alphabet[esp_random() % (sizeof(alphabet) - 1)];
    out[n] = '\0';
}

bool saveSettings() {
    size_t n = omobdSettingsToJson(settings, smallBuf, sizeof(smallBuf), true);
    if (!n) return false;
    return prefs.putString("settings", smallBuf) == n;
}

void loadSettings() {
    String stored = prefs.getString("settings", "");
    settingsFromStore = omobdSettingsLoadOrDefault(stored.c_str(), stored.length(), settings,
                                                   settingsNote, sizeof(settingsNote));
    if (!settingsFromStore && stored.length()) {
        msg.control.print(F("OMOBD: stored settings rejected ("));
        msg.control.print(settingsNote);
        msg.control.println(F("), using defaults"));
    }
    bool dirty = false;
    if (settings.apPass[0] == '\0' && !settings.allowOpenAp) {
        generateApPassword(settings.apPass, sizeof(settings.apPass));
        dirty = true;
    }
    if (strcmp(settings.apSsid, "OMOBD") == 0) {
        // Make the default SSID unique per device so two cars don't collide.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(settings.apSsid, sizeof(settings.apSsid), "OMOBD-%02X%02X", mac[4], mac[5]);
        dirty = true;
    }
    if (dirty) saveSettings();
}

// ---------------------------------------------------------------------------
// Wi-Fi supervision
// ---------------------------------------------------------------------------

bool startAp() {
    const char* pass = settings.apPass[0] ? settings.apPass : nullptr;
    if (!pass && !settings.allowOpenAp) return false;
    bool ok = WiFi.softAP(settings.apSsid, pass);
    lastApRetry = millis();
    return ok;
}

void printAccessInfo() {
    msg.control.println();
    msg.control.println(F("OMOBD web dashboard"));
    if (net == Net::AP || net == Net::STA_AND_AP) {
        msg.control.print(F("  Wi-Fi network: ")); msg.control.println(settings.apSsid);
        msg.control.print(F("  Password:      "));
        msg.control.println(settings.apPass[0] ? settings.apPass : "(open network)");
        msg.control.print(F("  Open:          http://"));
        msg.control.println(WiFi.softAPIP().toString().c_str());
    }
    if (net == Net::STA || net == Net::STA_AND_AP) {
        msg.control.print(F("  Joined:        ")); msg.control.println(settings.staSsid);
        msg.control.print(F("  Open:          http://"));
        msg.control.println(WiFi.localIP().toString().c_str());
    }
}

void startNetwork() {
    WiFi.persistent(false);       // our settings live in NVS, not the IDF Wi-Fi store
    WiFi.setSleep(true);          // modem sleep keeps BLE and Wi-Fi coexisting
    if (settings.wifiMode == OMOBD_WIFI_OFF) { WiFi.mode(WIFI_OFF); net = Net::OFF; return; }
    if (settings.wifiMode == OMOBD_WIFI_STA_FALLBACK_AP) {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(settings.staSsid, settings.staPass);
        net = Net::STA_CONNECTING;
        netStateSince = millis();
        return;
    }
    WiFi.mode(WIFI_AP);
    net = startAp() ? Net::AP : Net::OFF;
    netStateSince = millis();
    if (net == Net::OFF) msg.control.println(F("OMOBD: access point failed to start, will retry"));
    printAccessInfo();
}

void superviseNetwork(uint32_t now) {
    switch (net) {
        case Net::STA_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                net = Net::STA; netStateSince = now; printAccessInfo();
            } else if (now - netStateSince > STA_CONNECT_TIMEOUT_MS) {
                // Couldn't join: keep trying in the background, but give the
                // driver a network to connect to right now.
                WiFi.mode(WIFI_AP_STA);
                net = startAp() ? Net::STA_AND_AP : Net::STA_CONNECTING;
                netStateSince = now;
                printAccessInfo();
            }
            break;
        case Net::STA:
            if (WiFi.status() != WL_CONNECTED) { net = Net::STA_CONNECTING; netStateSince = now; }
            break;
        case Net::OFF:
            if (settings.wifiMode == OMOBD_WIFI_AP && now - lastApRetry > AP_RETRY_MS) {
                WiFi.mode(WIFI_AP);
                if (startAp()) { net = Net::AP; printAccessInfo(); }
            }
            break;
        default:
            break;
    }
}

const char* netName() {
    switch (net) {
        case Net::AP: return "AP";
        case Net::STA_CONNECTING: return "STA_CONNECTING";
        case Net::STA: return "STA";
        case Net::STA_AND_AP: return "STA_AND_AP";
        default: return "OFF";
    }
}

// ---------------------------------------------------------------------------
// HTTP
// ---------------------------------------------------------------------------

// Changing anything needs HTTP basic auth: user "omobd", password = the AP password.
bool requireAuth() {
    if (settings.apPass[0] == '\0') return true;   // owner chose an open network
    if (server.authenticate("omobd", settings.apPass)) return true;
    server.requestAuthentication(BASIC_AUTH, "OMOBD");
    return false;
}

void sendJson(int code, const char* body) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(code, "application/json", body);
}

void sendError(int code, const char* why) {
    char esc[96];
    omobdJsonEscape(why, esc, sizeof(esc));
    snprintf(smallBuf, sizeof(smallBuf), "{\"error\":\"%s\"}", esc);
    sendJson(code, smallBuf);
}

void handleRoot() {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-cache");
    server.send_P(200, "text/html", (const char*)OMOBD_DASHBOARD_HTML_GZ, OMOBD_DASHBOARD_HTML_GZ_LEN);
}

float lastBoost = NAN, lastVacuum = NAN;

void handleLive() {
    static OmobdLiveChannel ch[MAX_INPUTS];
    uint8_t n = 0;
    for (uint8_t i = 0; i < MAX_INPUTS; i++) {
        const Input& in = inputs[i];
        if (in.pin == 0xFF) continue;
        const SensorInfo* si = getSensorByIndex(in.sensorIndex);
        float smin = si ? pgm_read_float(&si->minValue) : NAN;
        float smax = si ? pgm_read_float(&si->maxValue) : NAN;
        const ApplicationPreset* app = getApplicationByIndex(in.applicationIndex);
        const UnitsInfo* u = getUnitsByIndex(in.unitsIndex);
        float disp = in.value;
        if (u && isfinite(disp)) disp = disp * READ_UNITS_FACTOR(u) + READ_UNITS_OFFSET(u);
        OmobdLiveChannel& c = ch[n++];
        c.pin = in.pin;
        c.key = app ? READ_APP_NAME(app) : "";
        c.abbr = in.abbrName;
        c.label = in.displayName;
        c.unit = getUnitStringByIndex(in.unitsIndex);
        c.base = in.value;
        c.display = disp;
        c.health = omobdClassifyInput(in.flags.isEnabled, in.flags.hasReading, in.value, smin, smax, 0.05f);
        c.source = sourceForInput(in);
        c.severity = severityForInput(in);
    }
    const char* atmSrc = "FIXED";
    float atm = atmosphereBar(&atmSrc);
    OmobdLiveDerived d;
    d.atmBar = atm;
    d.atmSource = atmSrc;
    d.boostBar = lastBoost;
    d.vacuumBar = lastVacuum;
    d.engineHours = hourMeterActive ? omobdHourMeterHours(hourMeter) : NAN;
    d.oilState = omobdOilStateName(oilMon.state);
    d.oilMinBar = settings.oilCurveCount
        ? omobdOilMinForRpm(settings.oilCurve, settings.oilCurveCount, baseValue(settings.rpmPin))
        : NAN;
    OmobdLiveMeta meta;
    meta.seq = ++lastLiveSeq;
    meta.uptimeMs = millis();
    meta.mode = isInConfigMode() ? "CONFIG" : "RUN";
#if INCLUDE_RULES_ENGINE
    meta.systemSeverity = rulesSystemSeverity();
#else
    meta.systemSeverity = 0;
#endif
    meta.profileId = settings.profileId;
    meta.firmware = firmwareVersionString();
    size_t len = omobdWriteLiveJson(liveBuf, sizeof(liveBuf), meta, ch, n, d);
    if (!len) { sendError(500, "live document too large"); return; }
    sendJson(200, liveBuf);
}

void handleHistory() {
    const OmobdHistoryRing<HIST_LEN>* ring = nullptr;
    String key = server.arg("key");
    if (key == "boost") ring = &histBoost;
    else if (key == "vacuum") ring = &histVacuum;
    else if (server.hasArg("pin")) {
        long pin = server.arg("pin").toInt();
        for (auto& s : histSlots) if (s.pin == pin) { ring = &s.ring; break; }
    }
    if (!ring) { sendError(404, "no history for that channel"); return; }
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "");
    char chunk[512];
    int o = snprintf(chunk, sizeof(chunk), "{\"intervalMs\":%u,\"values\":[", (unsigned)settings.historyIntervalMs);
    for (size_t i = 0; i < ring->size(); i++) {
        float v = ring->at(i);
        char tmp[20];
        int l = isfinite(v) ? snprintf(tmp, sizeof(tmp), "%s%.3f", i ? "," : "", (double)v)
                            : snprintf(tmp, sizeof(tmp), "%snull", i ? "," : "");
        if (o + l >= (int)sizeof(chunk) - 4) { server.sendContent(chunk, o); o = 0; watchdogReset(); }
        memcpy(chunk + o, tmp, l); o += l;
    }
    o += snprintf(chunk + o, sizeof(chunk) - o, "]}");
    server.sendContent(chunk, o);
    server.sendContent("");
}

void handleStatus() {
    bool sdLogging = false;
#if INCLUDE_SD_LOGGING
    sdLogging = outputModules[OUTPUT_SD].enabled;
#endif
    char ssidEsc[72], noteEsc[96], profEsc[72];
    omobdJsonEscape(settings.apSsid, ssidEsc, sizeof(ssidEsc));
    omobdJsonEscape(settingsNote, noteEsc, sizeof(noteEsc));
    omobdJsonEscape(settings.profileId, profEsc, sizeof(profEsc));
    snprintf(smallBuf, sizeof(smallBuf),
             "{\"firmware\":\"%s\",\"profile\":\"%s\",\"network\":\"%s\",\"apSsid\":\"%s\","
             "\"apClients\":%d,\"staIp\":\"%s\",\"rssi\":%d,\"freeHeap\":%lu,\"minFreeHeap\":%lu,"
             "\"uptimeMs\":%lu,\"mode\":\"%s\",\"sdLoggingAvailable\":%s,\"sdLogging\":%s,"
             "\"settingsFromStore\":%s,\"settingsNote\":\"%s\",\"pollMs\":%u}",
             firmwareVersionString(), profEsc, netName(), ssidEsc,
             (int)WiFi.softAPgetStationNum(),
             net == Net::STA || net == Net::STA_AND_AP ? WiFi.localIP().toString().c_str() : "",
             net == Net::STA || net == Net::STA_AND_AP ? (int)WiFi.RSSI() : 0,
             (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
             (unsigned long)millis(), isInConfigMode() ? "CONFIG" : "RUN",
#if INCLUDE_SD_LOGGING
             "true",
#else
             "false",
#endif
             sdLogging ? "true" : "false",
             settingsFromStore ? "true" : "false", noteEsc, (unsigned)settings.livePollMs);
    sendJson(200, smallBuf);
}

void handleGetSettings() {
    if (!omobdSettingsToJson(settings, smallBuf, sizeof(smallBuf), false)) { sendError(500, "settings too large"); return; }
    if (server.hasArg("download")) server.sendHeader("Content-Disposition", "attachment; filename=omobd-settings.json");
    sendJson(200, smallBuf);
}

void handlePostSettings() {
    if (!requireAuth()) return;
    const String& body = server.arg("plain");
    OmobdSettings next;
    char why[64] = "";
    if (!omobdSettingsFromJson(body.c_str(), body.length(), settings, next, why, sizeof(why))) {
        sendError(400, why);
        return;
    }
    bool wifiChanged = next.wifiMode != settings.wifiMode || strcmp(next.apSsid, settings.apSsid) ||
                       strcmp(next.apPass, settings.apPass) || strcmp(next.staSsid, settings.staSsid) ||
                       strcmp(next.staPass, settings.staPass);
    bool curveChanged = next.oilCurveCount != settings.oilCurveCount ||
                        memcmp(next.oilCurve, settings.oilCurve, sizeof(next.oilCurve)) != 0;
    settings = next;
    if (!saveSettings()) { sendError(500, "could not write settings"); return; }
    if (curveChanged) omobdOilMonitorReset(oilMon);
    if (settings.atmSource != OMOBD_ATM_KEY_ON_MAP) keyOnAtmBar = NAN;
    sendJson(200, wifiChanged ? "{\"ok\":true,\"note\":\"Wi-Fi changes apply after a restart\"}"
                              : "{\"ok\":true}");
}

// Print adapter that streams preOBD's SYSTEM DUMP JSON to the HTTP client.
class ChunkPrint : public Print {
public:
    size_t write(uint8_t c) override {
        buf_[n_++] = (char)c;
        if (n_ == sizeof(buf_)) flushOut();
        return 1;
    }
    void flushOut() {
        if (n_) { server.sendContent(buf_, n_); n_ = 0; watchdogReset(); }
    }
private:
    char buf_[512];
    size_t n_ = 0;
};

void handleExportConfig() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendHeader("Content-Disposition", "attachment; filename=omobd-preobd-config.json");
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "");
    ChunkPrint cp;
    dumpConfigToJSON(cp);
    cp.flushOut();
    server.sendContent("");
}

void handleLogging() {
#if INCLUDE_SD_LOGGING
    if (!requireAuth()) return;
    String body = server.arg("plain");
    bool on;
    if (body.indexOf("\"enabled\":true") >= 0) on = true;
    else if (body.indexOf("\"enabled\":false") >= 0) on = false;
    else { sendError(400, "expected {\"enabled\":true|false}"); return; }
    // Runtime only, same as OUTPUT SD_Log ENABLE without SAVE.
    if (!setOutputEnabled("SD_Log", on)) { sendError(500, "SD logging not available"); return; }
    sendJson(200, on ? "{\"ok\":true,\"sdLogging\":true}" : "{\"ok\":true,\"sdLogging\":false}");
#else
    sendError(501, "this build has no SD logging");
#endif
}

void handleNotFound() { sendError(404, "not found"); }

}  // namespace

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

void omobdWebInit() {
    for (auto& s : histSlots) s.pin = OMOBD_PIN_NONE;
    prefs.begin("omobd", false);
    loadSettings();
    omobdOilMonitorReset(oilMon);
    uint64_t storedMs = prefs.getULong64("runMs", 0);
    omobdHourMeterInit(hourMeter, storedMs);

    startNetwork();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/live", HTTP_GET, handleLive);
    server.on("/api/history", HTTP_GET, handleHistory);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/settings", HTTP_GET, handleGetSettings);
    server.on("/api/settings", HTTP_POST, handlePostSettings);
    server.on("/api/config/export", HTTP_GET, handleExportConfig);
    server.on("/api/logging", HTTP_POST, handleLogging);
    server.onNotFound(handleNotFound);
    server.begin();
}

void omobdWebUpdate(uint32_t now) {
    superviseNetwork(now);
    server.handleClient();

    // Calculated values
    maybeCaptureKeyOnAtm();
    const char* atmSrc;
    float atm = atmosphereBar(&atmSrc);
    lastBoost  = omobdGaugeFromAbsolute(baseValue(settings.boostMapPin), atm);
    lastVacuum = omobdVacuumFromAbsolute(baseValue(settings.vacuumMapPin), atm);

    float rpm = baseValue(settings.rpmPin);
    hourMeterActive = settings.rpmPin != OMOBD_PIN_NONE;
    if (hourMeterActive) {
        omobdHourMeterUpdate(hourMeter, rpm, settings.runningRpm, now, 2000);
        if (omobdHourMeterShouldPersist(hourMeter, 6UL * 60UL * 1000UL)) {
            prefs.putULong64("runMs", hourMeter.runMs);
            omobdHourMeterMarkPersisted(hourMeter);
        }
    }
    OmobdOilConfig oc{settings.oilCurve, settings.oilCurveCount, settings.runningRpm,
                      settings.oilWarmupMs, settings.oilDebounceMs};
    omobdOilMonitorUpdate(oilMon, oc, rpm, baseValue(settings.oilPressurePin), now);

    sampleHistory(now, lastBoost, lastVacuum);
}

#endif // OMOBD_WEB_DASHBOARD
