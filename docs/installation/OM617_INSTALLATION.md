# Installing OMOBD on an OM617 (factory Mercedes-Benz installation)

For an OM617 still in a Mercedes chassis (W115, W116, W123, W126 — confirm
yours) with its original instrument cluster and harness. For an engine
swapped into another vehicle, use the
[Jeep conversion guide](OM617_JEEP_CONVERSION.md) instead: its electrical
side is different.

> **Status:** these steps have not yet been carried out on a vehicle by the
> project. They follow standard practice; report what you find.
> **Safety:** work with the engine cold and the battery disconnected for any
> electrical work. Keep every factory gauge and warning light working and
> independent of OMOBD.

## 0. Plan

| Decide | Options |
|---|---|
| Variant | OM617.912 (NA) → `esp32s3-om617-na.*`; OM617.95x (turbo) → `esp32s3-om617-turbo.*` |
| What to fit first | Start with EGT, oil pressure, coolant temperature and battery voltage. Add the rest once those read correctly. |
| Where the unit lives | Cabin, behind the glovebox or under the dash, away from heater outlets |
| How the wiring reaches the engine | Existing firewall grommet if there's room; otherwise a new grommet |

## 1. Bench first

Complete [Bench testing](../testing/BENCH_TESTING.md) with every sensor
connected on the bench before anything goes into the car.

## 2. Power and ground

1. Find an ignition-switched (run) 12 V feed in the fuse box. Use a fuse tap
   on a circuit that is off with the key out.
2. Inline **2 A** fuse at the tap.
3. Automotive 12 → 5 V converter with reverse-polarity and transient
   protection ([Hardware requirements](../hardware/HARDWARE_REQUIREMENTS.md#power)).
4. Ground the unit to the same point the instrument cluster uses or, better,
   run a dedicated ground wire to the engine block. Temperature and pressure
   senders return through the block, so the unit must share it.

## 3. Exhaust gas temperature

- **Turbo:** probe in the exhaust manifold **before the turbine** (manifold
  collector or a pre-turbine bung). **NA:** in the manifold outlet.
- Drilling and tapping the manifold is usually done **with the manifold off
  the engine**. Metal chips that reach a turbocharger's turbine wheel can
  destroy it; never drill a pre-turbine port in place without a reliable way
  to catch every chip.
- The probe tip should sit in the gas stream, roughly central in the passage,
  without reaching the far wall.
- Type-K extension wire to the MAX31855; no copper joins.

## 4. Boost and vacuum (turbo; vacuum on both)

- **Boost:** tee the MAP sensor's hose into an intake-manifold port. Use
  fuel/vacuum-rated hose, short runs, a fall back toward the manifold (so
  condensation drains away from the sensor), and mount the sensor in the
  cabin or a cool spot.
- **Vacuum:** tee the second sensor into the vacuum system where you want to
  watch it (pump outlet, reservoir, or a specific actuator line). Every tee is
  a possible leak: check the vacuum-operated functions (door locks, climate
  flaps, transmission modulator, shutoff if vacuum-operated) still work after
  fitting. **Don't tee into the engine shut-off vacuum line in a way that can
  leak** — a leak there can affect stopping the engine.
- Set the atmospheric reference in OMOBD settings (`atmosphere.source`):
  `FIXED` (sea level 1.01325 bar, or your local value), `INPUT` (a BME280),
  or `KEY_ON_MAP` (needs the RPM input to prove the engine is stopped).

## 5. Oil pressure

- Fit a tee at the oil pressure take-off so the **factory gauge line stays
  connected** and the OMOBD sender goes on the tee.
- Use pressure-rated fittings and thread sealant suitable for oil; check for
  leaks at idle and at temperature before driving.

## 6. Temperatures

- **Coolant:** a separate sender in an adapter or spare port. Don't wire to the
  factory gauge sender.
- **Oil:** sender in a filter-housing sandwich adapter, a tapped drain plug, or
  a spare gallery port. Immersed senders only for absolute readings; see
  [immersed vs surface](../hardware/SENSOR_COMPATIBILITY.md#immersed-vs-surface-mounted-temperature-sensors).
- **Transmission (optional):** sender in the pan or a cooler-line adapter.
- **Intake air (optional):** after the turbo/intercooler if fitted, otherwise in the intake.

## 7. RPM

- **Alternator W terminal** if present, through the protected circuit in
  [Signal conditioning](../hardware/SIGNAL_CONDITIONING.md#5-rpm).
- If the car has an electronic tachometer, its signal source and pulses per
  revolution are **not documented here** — measure with an oscilloscope before
  connecting anything, and never load that circuit.
- Calibrate: [RPM calibration](../calibration/RPM_CALIBRATION.md).

## 8. Load the configuration

Over USB:

```
CONFIG
JSON IMPORT
```

Paste `engine-profiles/OM617/configs/esp32s3-om617-turbo.preobd.json` (or `-na`),
then a line `END JSON`. Check and save:

```
LIST INPUTS
SAVE
RUN
```

Then in the browser dashboard: **Settings → Device settings**, paste the
matching `.omobd-settings.json`, enter the unit's Wi-Fi password, **Save to
unit**; and pick the profile under **Settings → Engine profile**.

## 9. Calibrate and set alarms

- [Sensor calibration](../calibration/SENSOR_CALIBRATION.md)
- [RPM calibration](../calibration/RPM_CALIBRATION.md)
- [Alarm thresholds](../calibration/ALARM_THRESHOLDS.md) — none are preset

## 10. Vehicle testing

Follow [Vehicle testing](../testing/VEHICLE_TESTING.md), starting stationary.
