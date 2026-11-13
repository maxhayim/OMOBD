# RPM calibration

OMOBD reads RPM with preOBD's `W_PHASE_RPM` sensor (pulse counting). The
reading is only as good as its calibration, and every OM installation differs.

## What sets the numbers

| Source | Parameters |
|---|---|
| Alternator W terminal | alternator **pole count**, **pulley ratio** (alternator ÷ crank speed), optional multiplier |
| Crank/flywheel pickup | pulses per revolution = trigger teeth (set poles = 2 × teeth, ratio 1.0) |
| Existing tach signal | pulses per revolution, measured |

preOBD's formula and defaults (12 poles, 3:1) are in the
[W-Phase RPM Guide](../guides/sensor-types/W_PHASE_RPM_GUIDE.md). The OM617
configs use the input on GPIO 15 with preOBD's default calibration, which is
**almost certainly not exactly right** for your engine.

## Procedure

1. **Measure the pulley ratio.** Crank pulley diameter ÷ alternator pulley
   diameter (effective diameter where the belt runs). Or mark both pulleys,
   turn the crank exactly 10 turns by hand (engine off, glow plugs and
   battery disconnected) and count alternator turns.
2. **Find the pole count.** From the alternator's datasheet/label if possible.
   Many automotive alternators have 12 poles, but check.
3. Set it (pin 15, poles, ratio, timeout ms, min, max RPM):
   ```
   CONFIG
   SET 15 CAL RPM 12 2.6 2000 100 6000
   SAVE
   RUN
   ```
   (12 and 2.6 are placeholders — use your values.)
4. **Compare against a reference** at a steady idle and a steady higher
   speed: a handheld optical (photo) tachometer on a reflective mark on the
   crank pulley, or a known-good diesel tach. Note both readings.
5. If they differ by a constant percentage (belt slip, pulley-diameter
   estimate), use the six-parameter form with a multiplier:
   ```
   SET 15 CAL RPM 12 2.6 1.03 2000 100 6000
   ```
   multiplier = reference RPM ÷ OMOBD RPM.
6. Recheck at idle and at speed. Record the values in your installation notes.

## Notes by RPM source

- **W terminal at idle:** some alternators produce little or no W output
  until they are charging; a reading that appears only above idle is a
  symptom of that, not of a calibration error.
- **Belt slip** under heavy electrical load makes W-terminal RPM read low.
  For hour-meter and oil-curve purposes this is usually acceptable; for exact
  RPM use a crank pickup.
- **VR pickups** produce weak signals at cranking speed; set the min RPM so
  cranking doesn't count as running (OMOBD's `runningRpm`, default 300).
