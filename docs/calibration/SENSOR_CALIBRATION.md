# Sensor calibration (OM617)

The sensor library's built-in curves come from manufacturers' data. Your
sender, wiring and ground make each installation different, so check every
input against a reference before trusting it. preOBD's
[Advanced Calibration Guide](../guides/configuration/ADVANCED_CALIBRATION_GUIDE.md)
has the full `SET <pin> CAL ...` reference.

## Atmospheric reference (boost and vacuum)

Boost and vacuum are **calculated** from absolute pressure and an
atmospheric reference. Pick one in OMOBD settings (`atmosphere.source`):

| Source | When | Notes |
|---|---|---|
| `FIXED` | default | `fixedBar` = 1.01325 at sea level. At altitude set your local *absolute* pressure (e.g. ~0.84 bar around 1,600 m) or boost will read low. |
| `INPUT` | a barometric sensor is fitted (BME280 `BAROMETRIC_PRESSURE` input) | follows weather and altitude; set `atmosphere.inputPin` |
| `KEY_ON_MAP` | no barometer | captures the boost MAP sensor at key-on **only when RPM proves the engine is stopped**; needs the RPM input. Falls back to `FIXED` until captured. |

**Check:** key on, engine off. Boost should read 0.00 ± your sensor's
tolerance, and the MAP input should equal local atmospheric pressure.

## Pressure inputs

1. **Zero:** engine off, no pressure (oil pressure line at rest, MAP at
   atmosphere). Note the reading.
2. **Span:** compare against a mechanical test gauge teed into the same point:
   oil pressure at hot idle and at a steady higher RPM; boost under steady load
   (passenger reading the gauge, not the driver).
3. For a 0.5–4.5 V sensor that is off by a constant ratio, confirm the
   divider ratio first (`SET <pin> DIVIDER <ratio>`). For a resistive VDO
   sender, confirm the bias resistor value and the ground.
4. Custom linear calibration: `SET <pin> CAL LINEAR <vmin> <vmax> <outmin> <outmax>`
   (see preOBD's reference for units).

## Temperature inputs

1. **Ambient check:** cold engine overnight; every immersed temperature should
   agree with ambient within a few degrees.
2. **Hot check:** compare coolant with an infrared thermometer at the
   thermostat housing **and** a known-good gauge; IR on bare metal reads low,
   so treat it as a lower bound.
3. For a reference point, a sender in boiling water (sea level ≈ 100 °C) on
   the bench is a good check of a thermistor curve.
4. Custom thermistor curves: `SET <pin> CAL BETA ...` or `SET <pin> CAL STEINHART ...`.

## EGT

- Room temperature first: should read close to ambient.
- MAX31855 needs no calibration of its own; errors come from wiring (copper
  joints, wrong extension wire polarity — yellow/red conventions differ by
  country) and probe placement.
- Reversed polarity shows as temperature falling as the exhaust heats.

## Battery voltage

Compare with a multimeter at the battery with the engine off and again
running; adjust `SET <pin> CAL VOLTAGE_DIVIDER <r1> <r2> <correction>`.

## Record it

Keep a calibration log (date, input, reference instrument, readings,
adjustment). It's what makes an installation report useful — and it's what
moves a capability from `proposed` to `tested` in a profile.
