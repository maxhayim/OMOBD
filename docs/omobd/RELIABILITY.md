# Reliability design

EDMS-II worked but was not consistently reliable. Its documented problems
(Bluetooth reconnects, malformed packets, a failing 1-Wire bus, water in
the engine-bay box) are listed in the
[history](../history/EDMS-II.md#5-reliability-problems-reported-in-the-thread).
This is how each reliability requirement is handled in OMOBD, and how
it's checked. "preOBD" means the behaviour is inherited unchanged.

| Requirement | Implementation | Checked by |
|---|---|---|
| Non-blocking sensor acquisition | preOBD's time-sliced loop: each sensor read at its own interval, no `delay()` in RUN | preOBD design; OMOBD adds no blocking calls |
| Sensor timeouts | RPM/speed inputs time out to "no signal" (`timeoutMs` in calibration); MAX31855 fault bits → NaN | preOBD drivers; `test_omobd_live` mock thermocouple stream |
| Disconnected sensor detection | NaN → `fault`; outside the sensor's own range ± 5 % → `out_of_range`; never shown as 0 | `test_omobd_derived` (health), `test_omobd_live` |
| Invalid measurement handling | Calculated values go `null` when any input is invalid; OBD encoder refuses non-finite values | `test_omobd_derived`, `test_omobd_presets` |
| ADC filtering / noise | RC filter per input ([signal conditioning](../hardware/SIGNAL_CONDITIONING.md)); preOBD ADC setup; rules have debounce and hysteresis | hardware validation pending |
| Persistent configuration validation | preOBD: EEPROM with CRC (`test_config_crc`). OMOBD settings: full validation on load and on every write; corrupt → defaults, reported in `/api/status` | `test_omobd_settings` |
| Watchdog recovery | preOBD hardware watchdog on every board; OMOBD feeds it during long streamed responses; IDF 5 support added | build; hardware validation pending |
| Error reporting | `/api/status` (settings origin, heap low-water mark, network), console messages, per-tile status | — |
| Communication reconnection | Browser polls with timeouts and keeps retrying; station mode reconnects and falls back to an AP; failed AP start retried every 30 s | hardware validation pending |
| Data logging integrity | preOBD SD logging (CONFIG mode never logs bench junk); OMOBD history keeps gaps as gaps | `test_omobd_live` (history ring) |
| Safe startup | Defaults are inert: no alarm rules from profiles, oil curve empty, nothing "fitted" until configured; settings fall back to defaults; Wi-Fi password generated per device | `test_omobd_settings` |
| Engine hours integrity | Only time with a valid RPM ≥ threshold counts; loop stalls > 2 s not credited; saved every 6 min of running (flash wear) | `test_omobd_derived` |

## No interference with the engine

- No OMOBD code drives a GPIO. Profiles define no relay Actions.
- Senders are additional to the factory ones; factory gauges and warning
  lamps stay on their own circuits.
- Vacuum and pressure tees are documented with leak checks, and the engine
  stop path is never tapped.
- If OMOBD crashes, reboots or loses power, the vehicle is unaffected.

## Memory

All OMOBD buffers are static: 10 KB live document, 2 KB scratch, history
12 channels × 600 samples + boost + vacuum (~37 KB). The `esp32s3_omobd`
build uses 42.6 % of RAM at link time. `minFreeHeap` in `/api/status` shows
the run-time low-water mark — part of the endurance test.
