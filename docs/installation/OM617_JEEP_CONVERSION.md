# OMOBD on an OM617 in a Jeep (engine conversion)

An OM617 swapped into a Jeep shares the engine-side work with the
[factory installation guide](OM617_INSTALLATION.md) (EGT, boost, vacuum, oil
pressure, temperatures). **Everything electrical is different**, and that is
what this guide covers.

**Assume nothing about the factory Mercedes harness or cluster.** Most swaps
keep neither; some keep parts of each; many add aftermarket gauges. Trace
what your vehicle actually has before connecting anything.

> **Status:** not yet carried out by the project. Conversions vary widely;
> treat every step as "verify on your vehicle".

## 1. Survey the conversion first

Write these down; they decide the wiring:

| Question | Why it matters |
|---|---|
| Which OM617 variant (stamped number), NA or turbo? | Picks the profile config |
| Which alternator: Mercedes, Jeep, or aftermarket? Does it have a **W** terminal? | RPM source |
| What drives the tachometer now (if there is one)? | Don't load that signal; you may share it via a buffer |
| What gauges exist (Jeep cluster, aftermarket, none)? Which senders do they use? | Never share a sender between a gauge and OMOBD |
| Where is the engine grounded? Strap to chassis? To battery negative? | Sender readings depend on a solid block ground |
| How is the engine stopped (key-off fuel shutoff solenoid, vacuum shutoff, manual lever)? | Keep it independent of OMOBD; don't tee vacuum into a vacuum shut-off line |
| Glow-plug controller type and wiring | High current; keep OMOBD wiring away |
| Transmission (Jeep manual/automatic, transfer case) | Transmission temperature location; speed sensor options |
| Battery location, cable sizes | Ground return and voltage readings |

## 2. Grounding (most common cause of bad readings in swaps)

- Confirm a heavy strap **engine block → chassis** and **engine block →
  battery negative**. Many swaps rely on one or the other.
- Ground OMOBD to the **engine block**, since the temperature and pressure
  senders return through it.
- Measure: with the engine running at fast idle and the alternator charging,
  the voltage between the engine block and the battery negative post should
  be close to zero (tens of millivolts). Hundreds of millivolts means a poor
  ground that will skew every resistive sender.

## 3. Power

Jeep fuse boxes differ by model and year. Use an ignition-switched (run)
circuit through an add-a-fuse, **2 A** inline fuse, automotive buck
converter, as in [Hardware requirements](../hardware/HARDWARE_REQUIREMENTS.md#power).
If the swap has its own relay-switched ignition circuit, use that.

## 4. RPM in a conversion

In order of preference:

1. **Alternator W terminal**, through the protected conditioning circuit.
   Many diesel swaps fit an alternator with W for an aftermarket tach. You
   must calibrate pole count and pulley ratio for *your* alternator and
   pulleys ([RPM calibration](../calibration/RPM_CALIBRATION.md)).
2. **A dedicated pickup** (hall or VR sensor on the crank pulley, flywheel,
   or a trigger wheel) with its own conditioning. This needs mechanical work
   but gives the cleanest signal.
3. **The existing tachometer signal**, only after measuring it with an
   oscilloscope and only through a high-impedance buffer.

Jeep crank or cam sensors from the original petrol engine are **not**
present on the OM617.

## 5. Vehicle speed

The OM617 has nothing to do with vehicle speed. Options depend on the Jeep:
transmission/transfer-case vehicle speed sensor output (measure it first),
or a hall sensor on the driveshaft. Use preOBD's `HALL_SPEED` sensor; see the
[Hall speed guide](../guides/sensor-types/HALL_SPEED_GUIDE.md).

## 6. Senders

Same as the factory guide, with one Jeep-specific point: preOBD's library
contains **Jeep petrol-engine senders** (`JEEP_4_0_*`, `JEEP_CJ_*`, `JEEP_RENIX_CTS`).
Those describe Jeep senders, **not** anything on an OM617. Only use them if
that exact sender is physically installed in your engine and you have
confirmed its part.

## 7. Everything else

Configure, calibrate and test exactly as in the
[factory guide](OM617_INSTALLATION.md#8-load-the-configuration), then follow
[Vehicle testing](../testing/VEHICLE_TESTING.md). In the vehicle-testing log,
record the conversion details from step 1: they are what makes your report
useful to the next person.
