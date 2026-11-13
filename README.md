<h1 align="center">OMOBD</h1>

<p align="center"><b>Modern engine monitoring and virtual OBD-style diagnostics for classic Mercedes-Benz OM diesel engines.</b></p>

<p align="center">
  <a href="LICENSE"><img alt="License: MIT + NonCommercial" src="https://img.shields.io/badge/license-MIT%20%2B%20NonCommercial-blue"></a>
  <img alt="Status: development" src="https://img.shields.io/badge/status-development-orange">
  <img alt="Platform: ESP32-S3" src="https://img.shields.io/badge/platform-ESP32--S3-informational">
  <a href="https://github.com/preobd/preOBD"><img alt="Fork of preOBD" src="https://img.shields.io/badge/fork%20of-preOBD-lightgrey"></a>
</p>

OMOBD watches an OM617 (and, later, its OM-family relatives) the way a
modern engine is watched: exhaust gas temperature, boost, vacuum, oil
pressure and temperatures, coolant, RPM and charging voltage, shown live in
any phone or tablet browser over the unit's own Wi-Fi, with history graphs,
alarms and logging. It runs with no internet connection.

OMOBD is a fork of **[preOBD](https://github.com/preobd/preOBD)** by Eric
Nichols and the preOBD contributors, and it continues the **EDMS-II** engine
instrumentation project by **Paul (PeachParts: mach4 / Idahobenz)**. See
[Acknowledgments](#acknowledgments).

> **Monitoring only.** OMOBD never controls fuel, starting, shutdown or any
> other engine function, and it adds nothing to the factory gauges' circuits.
> Keep your mechanical gauges as the primary instruments. Read the
> [DISCLAIMER](DISCLAIMER.md).

<h2>Screenshots</h2>

No screenshots yet: they will be added from a real OM617 installation, not
mock-ups. The dashboard is `dashboard/index.html`.

## Features

**From preOBD (reused, unchanged):**
- Sensor drivers: K-type thermocouples (MAX31855, MAX6675), VDO/Bosch/generic
  NTC senders, 0.5–4.5 V pressure sensors, MPX MAP sensors, resistive pressure
  senders, battery voltage, alternator W-terminal RPM, hall-effect speed, BME280
- Per-input calibration, alarm rules with warning tier, warm-up and debounce
- OBD-II answers over CAN (ELM327 adapters, Torque, OBD Fusion), ELM327
  emulation over Bluetooth LE, RealDash streaming, SD-card and serial logging
- Configuration over USB serial, JSON export/import, EEPROM storage, watchdog
- Runs on ESP32-S3, Teensy 4.x/3.6, Feather STM32F405, Arduino Mega 2560

**Added by OMOBD:**
- **Wi-Fi web dashboard** served by the ESP32-S3 itself — phone, tablet or desktop;
  dial/bar/number gauges you can add, remove, resize and reorder across pages;
  history graphs; alarm and sensor-status indicators; logging control;
  configuration backups. No app, no cloud.
- **Engine profiles** (`engine-profiles/`): OM617 (naturally aspirated and
  turbo) with ready-to-import sensor configs; planned profiles for OM615,
  OM616, OM601, OM602, OM603, OM604, OM605 and OM606
- **Calculated values, always labelled as calculated:** boost (MAP − atmosphere),
  vacuum depth, engine hours, with a configurable atmospheric reference
  (fixed, barometer input, or captured at key-on)
- **RPM-dependent low oil pressure advisory** — off until you enter a validated curve
- New preOBD applications `VACUUM` and `ENGINE_BAY_TEMP` (OBD-style PIDs 0xB8/0xB9)
- Reliability work: sensor-fault and out-of-range detection shown as such
  (never as 0), stale-connection banner, validated settings with safe
  fallback, Wi-Fi supervision with fallback access point
- ESP32 watchdog builds on both Arduino-ESP32 2.x and 3.x

## Requirements

- ESP32-S3 development board (ESP32-S3-DevKitC-1 N8R8 or similar)
- Sensors and signal conditioning for what you want to measure — see
  [Hardware requirements](docs/hardware/HARDWARE_REQUIREMENTS.md)
- [PlatformIO](https://platformio.org/) to build and flash
- A fused, ignition-switched 12 V supply with an automotive-grade regulator

## Installation and Setup

1. Read [Hardware requirements](docs/hardware/HARDWARE_REQUIREMENTS.md) and
   [Signal conditioning](docs/hardware/SIGNAL_CONDITIONING.md).
2. Build and flash:
   ```bash
   pio run -e esp32s3_omobd -t upload
   ```
3. Bench-test with the [bench testing guide](docs/testing/BENCH_TESTING.md).
4. Install on the engine:
   [OM617 in a Mercedes](docs/installation/OM617_INSTALLATION.md) or
   [OM617 in a Jeep conversion](docs/installation/OM617_JEEP_CONVERSION.md).

## Usage

At start-up the unit prints its Wi-Fi network name and a per-device password
on the USB console. Join that network and open `http://192.168.4.1`.

Load an engine profile's sensor configuration over USB:

```
CONFIG
JSON IMPORT
```

Paste `engine-profiles/OM617/configs/esp32s3-om617-turbo.preobd.json`, finish with a line
`END JSON`, check with `LIST INPUTS`, then:

```
SAVE
RUN
```

The profile configs contain **no alarm limits**. Add your own once you have
validated figures — see [Alarm thresholds](docs/calibration/ALARM_THRESHOLDS.md).

## How It Works

```
 senders ─► signal conditioning ─► ESP32-S3 ──► preOBD core (read, calibrate, alarm, log, OBD/CAN/BLE)
                                        └──► OMOBD (calculated values, history) ─► Wi-Fi ─► browser
```

OMOBD's code lives in `src/omobd/` and runs in the same loop as preOBD; the
preOBD core is reused rather than duplicated. Details:
[Web dashboard and API](docs/omobd/WEB_DASHBOARD.md),
[Virtual OBD](docs/omobd/VIRTUAL_OBD.md),
[Reliability](docs/omobd/RELIABILITY.md),
[Development guide](docs/omobd/DEVELOPMENT.md).

## Known Limitations

- **No hardware validation yet.** The firmware builds and the host tests pass;
  nothing has been run on an OM617. See
  [Hardware validation status](docs/testing/HARDWARE_VALIDATION_STATUS.md).
- EDMS-II's original code and PCB files are not available; the
  [history page](docs/history/EDMS-II.md) is reconstructed from Paul's forum posts.
- No engine thresholds are supplied; every warning limit is yours to set.
- DS18B20 sensors and fuel-flow measurement are not supported yet
  ([Roadmap](ROADMAP.md)).
- An OM617 has no ECU: OBD-style data is synthetic and there are no fault codes.

## Compatibility

- [Engine compatibility matrix](docs/om-engines/ENGINE_COMPATIBILITY.md)
- [Sensor compatibility matrix](docs/hardware/SENSOR_COMPATIBILITY.md)
- [Troubleshooting](docs/omobd/TROUBLESHOOTING.md) (and preOBD's [Troubleshooting](docs/TROUBLESHOOTING.md))

## License

OMOBD inherits preOBD's **MIT + NonCommercial License** ([LICENSE](LICENSE)).
Personal, educational and other non-commercial use, modification and
publication are allowed with attribution. **Commercial use is not permitted**
without written permission from the preOBD copyright holder. OMOBD is not
"open source" in the OSI sense. Details, and the status of EDMS-II material:
[docs/LICENSING.md](docs/LICENSING.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Especially wanted: verified OM engine
data (with sources), installation reports, and anything that helps recover
EDMS-II material (with Paul's permission).

## Related Projects

- [preOBD](https://github.com/preobd/preOBD) — the upstream project
- [preOBD Web Bluetooth app](https://preobd.github.io/preOBD/) — works with OMOBD's BLE link too

## Acknowledgments

- **preOBD** — Copyright (c) 2020–2025 Eric Nichols and the preOBD
  contributors. The sensor library, calibration, alarm engine, OBD-II/CAN/BLE
  outputs, CLI, storage and test suite are theirs.
- **EDMS-II** — **Paul** (PeachParts **mach4 / Idahobenz**) for the original OM
  engine instrumentation concept, experimentation and documentation, and
  for permission to build on it. [History](docs/history/EDMS-II.md).
- Mercedes-Benz is a trademark of Mercedes-Benz Group AG. Jeep is a trademark
  of FCA US LLC. OMOBD is not affiliated with or endorsed by either.
