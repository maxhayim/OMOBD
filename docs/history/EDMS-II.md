# EDMS-II: the engine instrumentation project OMOBD builds on

OMOBD continues work that **Paul** (PeachParts username **mach4**, later
**Idahobenz**) started and documented in the PeachParts Diesel Discussion
thread **"Engine Instrumentation Project"** (10 pages, January 2014 – May 2016):

- Thread: <https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project.html>
- EDMS-II PCB development starts on [page 6][p6] and continues on [page 7][p7].

Paul gave permission for his earlier work to be used as the foundation and
starting point for a new monitoring system. Credit for the original concept,
the experimentation and the documented implementation is his.

> **How this page was written.** All 10 pages of the thread were read on
> 2026-11-13. Everything below is a **paraphrase** with a link to the post
> it comes from (`#N` = the forum's post number). No post text, photos,
> drawings or PCB files are copied ([permissions](#7-permissions-and-redistribution)).
> Three labels are used:
>
> | Label | Meaning |
> |---|---|
> | **Thread** | Paul said this in the thread (post linked). |
> | **Stated** | Given to OMOBD directly (project brief relaying Paul), and not found in the thread. |
> | **OMOBD** | A design decision for OMOBD, not a claim about EDMS-II. |
>
> The thread shows photos and a video of the system, which are not
> reproduced here. Readers should look at them on the forum.

---

## 1. Timeline

| Date | What happened | Post |
|---|---|---|
| 2014-01-21 | Project announced. It follows on from Paul's electric vacuum pump project. The goal is an electronic "gauge cluster" for his diesel R107: EGT, boost, oil temp, trans temp, intake temp and vacuum, sent from an Arduino over Bluetooth to an old Android phone. | [#1][p1] |
| 2014-01 | Sensor plan described; Arduino IDE (C) on the microcontroller, Processing (Java) for the Android app. | [#4][p1], [#19][p2] |
| 2014-01/02 | Weeks of Bluetooth trouble. Data finally flows, with 85–90 % of packets good at first. The first end-to-end run with simulated data comes on 02-06. | [#26][p2], [#33][p3], [#36][p3] |
| 2014-02 | Moved from breadboard to a prototype board; MAP sensors tested and calibrated; mounted in the car. | [#37][p3], [#39][p3], [#41][p3] |
| 2014-02-21 | EGT probe fitted; Paul calls revision 1 complete. He says he won't publish the code. | [#49][p4] |
| 2014-09 → 10 | HTC phone dies, replaced by a cheap phablet. The display is mounted in the steering-wheel horn pad. | [#54][p4], [#55][p4] |
| 2014-12 | Water from a pressure wash gets into the electronics box (recovered). Fuel pressure and battery voltage added. | [#59][p4], [#60][p4] |
| 2015-04 | DS18B20 temperatures drop out after ~5 min; a flaky sensor is suspected. A screen-capture video is posted. | [#62][p5] |
| 2015-07 | Oil temperature sensor moved into the sump drain plug. | [#69][p5], [#73][p5], [#75][p5] |
| **2015-10-16** | **"EDMS-II" (Engine Data Monitoring System II)** named. Paul plans a PCB that others could build as a DIY project, but not a product. | [#86][p6] |
| 2015-10 → 12 | PCB rev 1 designed in Fritzing and made. A Galaxy Tab 4 7" display is fitted in the horn pad. PCB rev 2 is made, assembled, and installed in the car on 2015-12-09. | [#91][p7]–[#105][p7], [#133][p9]–[#135][p9] |
| 2015-11 → 2016-01 | MPG-computer extension started (fuel flow + distance); not finished in the thread. | [#104][p7]–[#132][p9], [#140][p10] |
| 2015-12 | Steampunk "Nixie tube" display screens. | [#136][p10], [#137][p10] |
| 2016-05-03 | Paul's last post in the thread. | [#144][p10] |

## 2. The vehicle

| | Detail | Source |
|---|---|---|
| Car | Paul's "diesel 107". His forum signature lists a "380SL (diesel)". | [#1][p1] |
| Engine | A turbocharged, California-emissions setup. Paul fitted a "test pipe" at the exhaust manifold junction, in place of the trap oxidizer. He sized the fuel-flow sensor using a "BMEP chart for the 617 engine" that he says applies to any turbocharged 617. | [#4][p1], [#120][p8] |
| Driveline | 2.47 rear axle ratio; earlier a manual transmission with a VR speed sensor on the driveshaft. | [#104][p7], [#120][p8] |

Reading these together, EDMS-II ran on a turbocharged **OM617** swapped
into an R107. That is an inference from the posts: Paul doesn't spell out
the engine number in one place.

## 3. Hardware

### Controller, enclosure and power

| Item | Revision 1 (2014) | EDMS-II PCB (2015) | Source |
|---|---|---|---|
| Microcontroller | Arduino Pro Mini | Arduino Nano | [#1][p1], [#86][p6], [#91][p7] |
| Bluetooth | HC-05 module (5 V) | HC-05. New modules turned out to be 3.3 V with reversed pinout, so a level shifter was added in rev 2. | [#93][p7], [#97][p7], [#103][p7] |
| Board | Prototype board, soldered | Custom PCB designed in Fritzing; two revisions | [#37][p3], [#94][p7], [#105][p7] |
| Location | **Engine bay**, front, driver's side, far from the turbo, inside a gutted plastic Mercedes relay box. The box is plastic so Bluetooth reaches the cabin. | Same location, new case, 15-pin D-sub connector | [#28][p2], [#97][p7], [#135][p9] |
| Connector | The relay box's 12-pin Mercedes plug, 10 wires used: 12 V, ground, 1-Wire signal and ground, 2 × EGT, 2 MAP signals with shared 5 V and ground | D-sub 15 | [#28][p2], [#97][p7] |
| Power | On-board power supply. Rev 2 switched to a sturdier converter "because of dirty car power". | buck converter | [#37][p3], [#97][p7] |
| Board temperature | Monitored with an on-board DS18B20. Typically 13–25 °F above ambient. | same | [#41][p3], [#51][p4], [#66][p5], [#67][p5] |
| Expansion | — | 2 spare analog + 3 spare digital inputs. A microSD card was designed in, then removed. | [#92][p7], [#94][p7] |

### Sensors

| Measurement | Sensor and installation | Source |
|---|---|---|
| EGT (strictly, turbine inlet temperature) | K-type thermocouple with a **MAX31855** amplifier (Adafruit breakout, which has level shifters; a cheaper board without them didn't work). The probe sits in a bung in the pre-turbine test pipe at the exhaust manifold junction. | [#4][p1], [#25][p2], [#36][p3], [#49][p4], [#97][p7] |
| Boost | **GM 3-bar MAP** sensor from a junkyard Saab ($2). It reads **absolute** pressure, so with the engine off at about 4,000 ft the boost gauge showed −1, because his calculation assumed sea-level pressure. | [#4][p1], [#36][p3], [#41][p3] |
| Vacuum | **GM 1-bar MAP** sensor, mounted by the vacuum lines on the driver's side. Main-line vacuum is watched all the time, and he can swap hoses to diagnose climate control, locks and the VCV. Vacuum drops sharply under braking. | [#4][p1], [#41][p3] |
| Oil, transmission, intake and board temperatures | **DS18B20** digital sensors on one 1-Wire bus. Oil and trans were **clamped to the cooler lines** in epoxied aluminium blocks, to avoid adding leak points. Intake air is in the air filter housing. Each sensor has a unique ID and had to be mapped to its gauge, which he did by heating each one with a torch. | [#19][p2], [#33][p3], [#42][p3], [#48][p4] |
| Oil temperature (later) | DS18B20 in a hollowed **oil drain plug**, held by magnets. Readings matched the cooler-line position closely. Another member argued neither position contacts the oil, and Paul then relied on the readings as relative values. | [#69][p5], [#73][p5], [#75][p5], [#77][p6], [#79][p6] |
| Fuel pressure | Pressure transducer, about $20, details in a separate thread ([362989][fp]) | [#59][p4], [#60][p4], [#83][p6] |
| Battery voltage | Simple resistor divider for 0–50 V, chosen so spikes can't damage an analog pin | [#59][p4], [#60][p4] |
| Current | Hall-effect ammeter module considered for charge/discharge and maybe glow-plug current. Module bought; not reported as installed. | [#96][p7], [#140][p10] |
| RPM and road speed | **Not implemented.** He says in 2015 that he has no RPM or speed data. VR sensors were discussed, and an LM1815 VR interface chip was bought for road speed in the MPG work. | [#12][p1], [#92][p7], [#104][p7] |

### MPG computer (unfinished)

Paul's goal was to measure only the fuel the engine burns. The injection
return would go to a vented **float bowl** instead of the tank. A low-pressure
pump would refill the bowl through its float valve, and one flow meter would
read that refill. He had used the same method with a Zemco trip computer on a
Perkins-diesel Vega. A second Arduino would count the fuel-flow and distance
pulses and pass them to the main board over I²C. The first flow meter was
unusable; a second one arrived in January 2016. The thread ends before the
system was installed. Other members questioned the float bowl in a long
exchange (venting, lift-pump behaviour, fuel heating).
([#104][p7]–[#132][p9], [#133][p9], [#140][p10])

## 4. Software

| Part | Detail | Source |
|---|---|---|
| Arduino | Arduino IDE (C/C++). Sends a **comma-separated text string**, with marker bytes at the start and end so incomplete packets can be detected and thrown away. | [#4][p1], [#28][p2], [#42][p3] |
| Sampling | Boost and vacuum 5×/s; DS18B20 temperatures every **10 s** because each read is slow. Reading them every cycle had cut the packet rate to about 2/s. Initially 4 readings/s; later doubled. | [#26][p2], [#42][p3], [#60][p4] |
| Bluetooth | Based on the "Arduino Basics: Bluetooth Android Processing" tutorial code. A higher baud rate made it more stable. Link is one-way only, Arduino → tablet. | [#28][p2], [#42][p3], [#95][p7], [#104][p7] |
| Android app | Written in **Processing** (Android mode). The gauge faces are images, and the app draws needles and numbers over them. Screens: white and dark analog sets, large boost/TIT, digital readout, linear "ribbon" temperatures, raw-data debug, steampunk Nixie. Also a "connected / receiving" indicator. Each new display resolution meant redrawing every screen. | [#1][p1], [#41][p3], [#42][p3], [#44][p3], [#54][p4], [#97][p7], [#136][p10] |
| Logging, alarms | **None** (Paul's answer in 2015). He suggested min/max values on the display instead of SD logging. | [#64][p5], [#92][p7] |
| Source code | **Not published.** Paul called it too messy to share, and later offered only snippets or possibly the Arduino code. The brief says the source **may be lost**; nothing in the thread contradicts that. | [#48][p4], [#49][p4], [#85][p6]; Stated |

## 5. Reliability problems reported in the thread

| Problem | What Paul found or did | Source |
|---|---|---|
| Bluetooth data drops | Code dropped data even though the raw stream arrived intact. He suspected timing and added delays; framing bytes and a higher baud rate improved it. | [#26][p2], [#42][p3] |
| **Bluetooth won't reconnect** | After a dropped link it often wouldn't reconnect, and sometimes locked up until the phone was rebooted. He later blamed much of the early trouble on the old HTC phone; the replacement tablet was "totally reliable". | [#48][p4], [#51][p4], [#60][p4] |
| One-wire temperature bus | All DS18B20 readings failed after about 5 minutes; the trans sensor was suspected. | [#62][p5] |
| Malformed packets | A bad packet that other screens ignored crashed the new Nixie screen until he added error handling. | [#137][p10] |
| Water ingress | Pressure-washing the engine bay put water in the box and stopped the Arduino. It recovered after drying and cleaning with acetone; he added drain holes. | [#59][p4] |
| Surface temperature sensing | Clamped and drain-plug oil readings were questioned as reading low. He switched to treating them as relative. | [#69][p5]–[#79][p6] |
| Parts changing under him | HC-05 pinout and voltage changed; some MAX31855 boards had no level shifting; the power converter was inadequate; a Fritzing footprint shorted to the ground plane; DS18B20 silkscreen was backwards. | [#93][p7], [#97][p7], [#134][p9] |
| Engine-bay heat | Monitored rather than a problem in his front-of-bay position, but he noted a different location might not be fine. | [#42][p3], [#66][p5] |

Advice from other members recorded in the thread that OMOBD also follows:
watch for **ground loops on the EGT thermocouple**. One member's readings
worked on laptop power but not on the car's ([#18][p2], winmutt). Another
member suggested engine hours counted from RPM ([#7][p1], sixto).

## 6. Lessons carried into OMOBD

| EDMS-II experience | OMOBD response | Where |
|---|---|---|
| Bluetooth pairing/reconnect failures and a custom Android app were the biggest pain | The unit runs its own Wi-Fi network and any browser shows the dashboard, with nothing to install. The page polls, so a dropped link recovers on its own and never stalls the unit; the dashboard shows "connection lost" rather than freezing. **OMOBD** | `src/omobd/omobd_web.cpp`, `dashboard/index.html` |
| Malformed data crashed a screen | JSON from the unit; the browser validates every response; failed readings are `null`, never garbage. **OMOBD** | `omobd_live.cpp`, `dashboard/index.html` |
| Absolute MAP read −1 at altitude | Boost = MAP − a **configurable atmospheric reference**: fixed local value, barometer, or MAP captured at key-on with the engine stopped. **OMOBD** | `omobd_derived.cpp`, [Sensor calibration](../calibration/SENSOR_CALIBRATION.md) |
| Slow sensors throttled the update rate | preOBD's per-input read intervals in a non-blocking loop. **OMOBD** (inherited) | `src/main.cpp` |
| No logging, no alarms | preOBD SD logging and alarm rules, plus a 10-minute history kept on the unit. **OMOBD** | `docs/omobd/WEB_DASHBOARD.md` |
| Surface-mounted temperature readings were disputed | Docs separate immersed and surface sensors, and say not to set absolute limits from surface readings. **OMOBD** | [Sensor compatibility](../hardware/SENSOR_COMPATIBILITY.md#immersed-vs-surface-mounted-temperature-sensors) |
| Water in the engine-bay box | OMOBD recommends mounting **in the cabin**, with wiring through the firewall. **OMOBD** | [Hardware requirements](../hardware/HARDWARE_REQUIREMENTS.md) |
| Dirty car power; a converter that wasn't good enough | Automotive buck converter, TVS diode, reverse-polarity protection and fuse documented. **OMOBD** | [Hardware requirements](../hardware/HARDWARE_REQUIREMENTS.md#power) |
| 3.3 V / 5 V mismatches (HC-05, MAX31855) | ESP32-S3 is 3.3 V throughout; every 5 V sensor goes through a divider. **OMOBD** | [Signal conditioning](../hardware/SIGNAL_CONDITIONING.md) |
| EGT ground-loop warning | Ungrounded probe recommended; thermocouple faults show as faults, not values. **OMOBD** | [Signal conditioning](../hardware/SIGNAL_CONDITIONING.md#4-thermocouple-egt) |
| DS18B20 was EDMS-II's main temperature sensor | preOBD has no DS18B20 driver yet; it's on the roadmap with non-blocking conversion. Until then OMOBD uses immersed thermistor senders. **OMOBD** | [Roadmap](../../ROADMAP.md) |
| No RPM, no speed | W-terminal or pickup RPM with calibration, used for engine hours and the oil-vs-RPM advisory. **OMOBD** | [RPM calibration](../calibration/RPM_CALIBRATION.md) |
| Source code never published | Public Git history, host tests and CI. **OMOBD** | this repository |

### Paul's measurements (his engine, not specifications)

These are figures Paul reported from **his** installation. They are recorded
here as history only. OMOBD does **not** use them as thresholds or defaults.

- First EGT result on a long 6 % climb: 622, which Paul then realised was
  °C because his code skipped the °F conversion ([#49][p4]).
- Oil temperature: about 150 °F in normal running, up to about 180 °F on long
  climbs on warm days, with both sensor positions. Another member questioned
  the method ([#75][p5], [#79][p6]).
- Electronics box: about 13–25 °F above ambient at the front of the engine bay ([#51][p4], [#67][p5]).
- His road-load fuel calculation: about 1.9 gal/h, about 31.5 mpg at 60 mph
  ([#120][p8]).

## 7. Permissions and redistribution

- Paul's permission (**Stated**) covers **using his work as a foundation**
  for OMOBD.
- It is **not** treated as permission to republish his **photos, drawings,
  PCB files or post text**. None are in this repository; this page
  paraphrases and links.
- Paul said in 2015 that he didn't want to sell EDMS-II as a product
  ([#82–#85][p6], [#86][p6], [#99][p7]). This fits OMOBD's non-commercial
  licence. Don't sell OMOBD hardware built on his design without asking him.
- If original files turn up (code, Fritzing/Gerber files, gauge artwork,
  photos), get Paul's explicit written permission and preferred licence
  before committing them, and record it in `docs/LICENSING.md`.
- **Stated only, not in the thread:** that Paul now posts as Idahobenz, that
  he supports an ESP32 + Wi-Fi + browser redesign, and his own view that the
  system was not consistently reliable. The thread does document the
  specific problems in [section 5](#5-reliability-problems-reported-in-the-thread).

## 8. Still open

- [ ] Ask Paul whether any code, Fritzing or Gerber files, or gauge artwork survive, and on what terms they could be shared.
- [ ] Ask Paul which photos from the thread, if any, he is happy to have reproduced here.
- [ ] Read the linked side threads: fuel pressure gauge ([362989][fp]), MPG computer ([351581][mpg]), oil cooler bypass discussion ([368682][oil]).
- [ ] Confirm the engine variant in Paul's car with him.

[p1]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project.html
[p2]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-2.html
[p3]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-3.html
[p4]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-4.html
[p5]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-5.html
[p6]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-6.html
[p7]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-7.html
[p8]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-8.html
[p9]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-9.html
[p10]: https://www.peachparts.com/shopforum/diesel-discussion/350205-engine-instrumentation-project-10.html
[fp]: https://www.peachparts.com/shopforum/diesel-discussion/362989-adding-fuel-pressure-gauge.html
[mpg]: https://www.peachparts.com/shopforum/diesel-discussion/351581-mpg-computer.html
[oil]: https://www.peachparts.com/shopforum/diesel-discussion/368682-running-oil-cooler-bypassed.html
