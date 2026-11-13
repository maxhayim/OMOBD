# EDMS-II: the engine instrumentation project OMOBD builds on

OMOBD continues work that **Paul** (PeachParts usernames **mach4**, later
**Idahobenz**) started and published on the PeachParts Diesel Discussion forum:

- Main thread: <https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project.html>
- Later development (page 7): <https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-7.html>

Paul gave permission for his earlier work to be used as the foundation and
starting point for a new monitoring system. Credit for the original concept,
the experimentation, and the documented implementation is his.

> **Status of this document — read first.**
> The forum thread itself has **not yet been reviewed** for this document.
> When this page was written, the build environment's network policy blocked
> `www.peachparts.com`, so nothing from the thread (posts, photos, schematics,
> part numbers, measurements) could be read or checked.
> Everything below is therefore split into three kinds of statement, and
> nothing has been filled in from guesswork:
>
> | Label | Meaning |
> |---|---|
> | **Stated** | Given to the OMOBD project directly (project brief from the repository owner, relaying Paul's account). Not yet cross-checked against the thread. |
> | **To research** | A question the thread should answer. Left blank on purpose. |
> | **OMOBD proposal** | A design decision made for OMOBD. Not a claim about EDMS-II. |
>
> The "Research checklist" at the end lists exactly what to extract once the
> thread can be read. Please contribute findings with post links.

---

## 1. What is stated about EDMS-II

| # | Statement | Source |
|---|---|---|
| S1 | The original developer is Paul, PeachParts user mach4 (now Idahobenz). | Stated |
| S2 | The project was documented in the PeachParts "Engine Instrumentation Project" thread (links above), including later pages. | Stated |
| S3 | The original system used **Arduino** hardware. | Stated |
| S4 | Data went from the Arduino to the display over **Bluetooth**. | Stated |
| S5 | The display was an **Android** app written in **Processing**. | Stated |
| S6 | The system worked but was **not consistently reliable** (Paul's own assessment). | Stated |
| S7 | Paul's **source code may no longer exist**. | Stated |
| S8 | Paul **supports modernising** the design around an **ESP32**, **Wi-Fi**, and a **browser** dashboard. | Stated |
| S9 | Paul has given **permission** to use his previous work as a foundation and starting point. | Stated |
| S10 | The measurements the OMOBD brief attributes to EDMS-II's scope: exhaust gas temperature (thermocouple), turbo boost, vacuum, several temperatures, oil pressure, fuel pressure, engine RPM, and electrical system voltage. | Stated (brief); which of these EDMS-II actually implemented is **to research** |

Nothing else about EDMS-II is asserted in this repository.

## 2. Original architecture (as far as is stated)

```
 sensors ──► Arduino ──Bluetooth──► Android phone/tablet
                                     (Processing app: gauges)
```

| Topic | What is known | Status |
|---|---|---|
| Controller | Arduino (model not recorded here) | Stated / model **to research** |
| Sensors and measurement methods | — | **To research** |
| Arduino implementation (sampling, filtering, libraries) | Source may be lost (S7) | **To research** |
| Bluetooth link (module, protocol, message format, rate) | — | **To research** |
| Android Processing dashboard (layout, gauges, logging) | — | **To research** |
| PCB development | — | **To research** |
| Circuit designs (thermocouple amp, pressure inputs, RPM conditioning, power) | — | **To research** |
| Calibration techniques | — | **To research** |
| Sensor placement and installation | — | **To research** |
| Reliability limitations — specific failure modes | Overall: not consistently reliable (S6) | Specifics **to research** |
| Improvements Paul proposed | ESP32, Wi-Fi, browser (S8) | Stated |

## 3. Lessons carried into OMOBD

Only S6 ("not consistently reliable") is known, not its causes. OMOBD
therefore treats reliability as a requirement everywhere instead of fixing
one known fault. The design responses below are **OMOBD proposals**; where a
link to EDMS-II is drawn, it is to the general reliability statement only.

| Common causes of unreliable DIY engine monitors | OMOBD response | Where |
|---|---|---|
| A dropped wireless link stalls or confuses the display | The browser polls; a sleeping phone or lost link never blocks the unit. The dashboard shows "connection lost" instead of freezing old numbers. | `src/omobd/omobd_web.cpp`, `dashboard/index.html` |
| Pairing/app-install friction (Bluetooth + custom app) | Any browser on the unit's own Wi-Fi network; nothing to install. preOBD's BLE OBD emulation stays available for OBD apps. | `docs/omobd/WEB_DASHBOARD.md` |
| A disconnected sensor shows a plausible number (often 0) | A failed reading is `null` with a "sensor fault" badge, never 0. Values outside the sensor's own range are flagged. | `omobd_derived.cpp` (`omobdClassifyInput`) |
| Blocking code (sensor waits, serial waits) | preOBD's time-sliced, non-blocking loop; OMOBD adds no blocking calls. | `src/main.cpp` |
| Lock-ups | preOBD's hardware watchdog on every board; OMOBD feeds it during long HTTP responses. | `src/core/watchdog.cpp`, `omobd_web.cpp` |
| Electrical noise and over-voltage on inputs | Signal conditioning is documented per input; nothing 12 V or 5 V goes straight to a GPIO. | `docs/hardware/SIGNAL_CONDITIONING.md` |
| Corrupt settings after a power cut | Settings are validated on every load; a corrupt store falls back to defaults and says so. | `omobd_settings.cpp` |
| Lost source code | Public Git history, host unit tests, and CI. | this repository |

## 4. Evolution toward OMOBD

| | EDMS-II (stated) | OMOBD |
|---|---|---|
| Controller | Arduino | ESP32-S3 (preOBD also runs on Teensy, STM32, Mega) |
| Link | Bluetooth | Wi-Fi access point on the unit (plus preOBD's BLE) |
| Display | Android app in Processing | Any browser: phone, tablet, laptop |
| Software base | Custom, possibly lost | preOBD (maintained, tested) + OMOBD additions |
| Engine knowledge | OM617-focused (to confirm) | Engine profiles: OM617 first, OM615/616/601–606 planned |

## 5. Permissions and redistribution

- Paul's permission (S9) covers **using his work as a foundation** for OMOBD.
- It is **not** treated as permission to republish his **photographs,
  drawings, PCB files, or post text**. None of those are copied into this
  repository. This page links to the thread instead.
- If original files are recovered (code, schematics, PCB artwork, photos),
  ask Paul — and any other contributor whose material is included — for
  explicit written permission and the licence they want, before committing
  them. Record that permission in `docs/LICENSING.md`.
- OMOBD as a whole is distributed under preOBD's MIT + NonCommercial licence
  (see `docs/LICENSING.md`). Paul's contributions should not be relicensed
  commercially without his agreement.

## 6. Research checklist (to complete from the thread)

When `peachparts.com` can be reached, work through every page of both links
and record, with a post link and date for each item:

- [ ] Arduino model; power supply and protection used in the vehicle
- [ ] Every sensor: part number, range, where and how it was installed
- [ ] Thermocouple amplifier chip; probe location (pre-turbine?); probe type
- [ ] Boost/MAP sensor part; absolute or gauge; atmospheric reference handling
- [ ] Vacuum measurement: which part of the vacuum system, what it was used to diagnose
- [ ] Temperature senders: immersed vs surface-mounted; locations
- [ ] Oil and fuel pressure senders; plumbing (tees, adapters)
- [ ] RPM source (alternator W terminal? other?) and the conditioning circuit
- [ ] Bluetooth module; data format and update rate
- [ ] Processing app: screens, gauges, logging, alarms
- [ ] PCB revisions; who made them; whether files survive
- [ ] Calibration steps and reference instruments used
- [ ] Each reliability problem reported, and any fix tried
- [ ] Measured results Paul published (EGT at load, boost levels, etc.) — record as his
      measurements on his engine, never as general specifications
- [ ] Which images/files Paul is happy to have reproduced, and under what terms

Until then, OMOBD's OM617 profile ships **no** thresholds or calibration
figures attributed to EDMS-II.
