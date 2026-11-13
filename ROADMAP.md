# OMOBD roadmap

## Milestone 1 — OM617 foundation (this release, 2026-11-13)

- [x] Fork of preOBD with history, upstream remote, licence and attribution preserved
- [x] ESP32-S3 OMOBD build (`esp32s3_omobd`), platform pinned; IDF 5 watchdog fix
- [x] Wi-Fi web dashboard: live gauges, editable multi-page layouts, history, alarms, sensor status, logging control, backups
- [x] Calculated boost/vacuum with configurable atmospheric reference; engine hours; oil-vs-RPM advisory
- [x] `VACUUM` and `ENGINE_BAY_TEMP` applications (PIDs 0xB8/0xB9)
- [x] OM617 profile (NA + turbo) with importable ESP32-S3 configs; planned profiles OM615/616/601–606
- [x] Profile validator; host tests for every OMOBD module; profile import through preOBD's importer
- [x] Installation (factory + Jeep), hardware, calibration, testing and history docs
- [ ] EDMS-II thread research ([checklist](docs/history/EDMS-II.md#6-research-checklist-to-complete-from-the-thread))

## Milestone 2 — hardware validation

- [ ] Bench test on ESP32-S3-DevKitC-1 (Wi-Fi + BLE coexistence, fault behaviour, 24 h endurance)
- [ ] First OM617 installation; calibration log; first `testedOn` records
- [ ] Phone/tablet browser matrix (iOS Safari, Android Chrome, desktop)
- [ ] Build and run on Arduino-ESP32 3.x (IDF 5 watchdog path)
- [ ] Confirm/fix ESP32-S3 I2C default pins for BME280; report upstream
- [ ] Offer the IDF 5 watchdog fix to preOBD

## Milestone 3 — sensors and data

- [ ] DS18B20 (1-Wire) temperature driver, non-blocking conversion, upstream-compatible
- [ ] 15–115 kPa absolute sensor in the library for vacuum resolution
- [ ] Optional: fuel supply + return flow measurement → consumption **estimate**, labelled as such
- [ ] Device-side long-term statistics (max EGT, max boost per drive)
- [ ] CSV download of history from the dashboard

## Milestone 4 — more engines

- [ ] Sourced identification data for OM615/616/601–606 (`verified: true` with sources)
- [ ] Configs for the next engine (likely OM602/OM603 turbo) and guidance for electronically assisted variants
- [ ] Profile-driven warning thresholds where owners contribute validated, sourced figures

## Ideas (not committed)

- Dashboard layouts stored on the unit (shared between devices)
- Reference PCB / shield with the documented conditioning circuits (subject to licence: non-commercial)
- mDNS name (`omobd.local`) where clients support it
