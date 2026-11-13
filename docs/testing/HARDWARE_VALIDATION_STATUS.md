# Hardware validation status

What has and has not been verified, as of the first OMOBD milestone
(2026-11-13). Update this page with every bench or vehicle test.

## Verified (no hardware involved)

| Item | How | Result |
|---|---|---|
| Upstream preOBD builds for ESP32-S3 | `pio run -e esp32s3dev`, platform pinned to espressif32 6.13.0 (local overlay) | ✅ builds (RAM 21.6 %, flash 37.6 %) |
| Upstream preOBD on unpinned platform | `pio run -e esp32s3dev` with `platform = espressif32` (resolved to 8.0.0 / Arduino-ESP32 3.x) | ❌ upstream build error in `watchdog_esp32.h` (IDF 5 API); fixed in OMOBD |
| OMOBD firmware builds for ESP32-S3 | `pio run -e esp32s3_omobd` (espressif32 6.13.0) | ✅ builds (RAM 42.6 %, flash 51.4 %) |
| preOBD host test suite with OMOBD changes | `pio test -e native` | ✅ all pass (see CHANGELOG for counts) |
| OMOBD host tests (derived values, settings, live JSON, history, mock streams, presets, profile import) | `pio test -e native -f "test_omobd_*"` | ✅ all pass |
| Engine profiles vs firmware registries | `tools/omobd/validate_profiles.py` + unit tests | ✅ |

## Not yet verified on hardware

| Area | Needed |
|---|---|
| Firmware boots on an ESP32-S3 board; Wi-Fi AP + BLE coexist | flash a DevKitC-1, run [bench tests](BENCH_TESTING.md) §1 |
| Web dashboard on real phones/tablets (iOS Safari, Android Chrome, desktop) | bench §1–3 |
| Station mode + fallback AP | bench, with a hotspot |
| Every sensor channel and its conditioning circuit | bench §2 |
| Fault behaviour (open/short sensors, link loss, power cycling) | bench §3 |
| 24 h endurance; heap stability | bench §4 |
| IDF 5 watchdog path (Arduino-ESP32 3.x) | build + run on espressif32 ≥ 7 (only the 6.x path has been built) |
| ESP32-S3 I2C pins for BME280 | see [sensor notes](../hardware/SENSOR_COMPATIBILITY.md#esp32-s3-i2c-pin-note) |
| Any OM617 installation (factory or Jeep) | [vehicle testing](VEHICLE_TESTING.md) |
| RPM calibration on an OM617 | reference tach comparison |
| BLE ELM327 emulation alongside Wi-Fi (Torque, OBD Fusion) | phone test with both links |

## Test log

| Date | Who | Hardware | What | Result |
|---|---|---|---|---|
| — | — | — | — | — |
