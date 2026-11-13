# Signal conditioning for OMOBD inputs

**Rule:** no GPIO ever sees 12 V, an unconditioned inductive or alternator
signal, or a 5 V sensor output directly. The ESP32-S3 is a 3.3 V part and is
**not 5 V tolerant**. Every circuit below has not yet been validated on a
vehicle; build it, measure it on the bench, then connect it.

## 1. Resistive senders (temperature, VDO pressure)

Sender (to engine ground) in series with a **bias resistor** from 3.3 V; the
ADC reads the junction. Follow preOBD's
[Bias Resistor Guide](../guides/hardware/BIAS_RESISTOR_GUIDE.md) for values per
sender, and add:

```
 3.3V ──[R bias]──┬──[1k]──┬── GPIO (A0..A9)
                  │        │
               sender    100nF
                  │        │
 engine block ────┴────────┴── unit GND (same block!)
```

The 1 kΩ + 100 nF low-pass (≈1.6 kHz) knocks down ignition-free but still
noisy diesel-bay interference and limits current if the wire chafes to 12 V
(add a 3.3 V clamp diode for that case).

**Do not** share the factory gauge's sender. The gauge's own current through
the sender changes what OMOBD reads and vice versa. Fit a second sender
(adapter, tee or spare boss).

## 2. 5 V ratiometric sensors (MPX4250AP, 0.5–4.5 V transducers)

```
 5V ───── sensor Vs
 GND ──── sensor GND
 sensor Vout ──[2.2k]──┬──── GPIO
                       │
                     [3.3k]   (+ 100nF to GND)
                       │
                      GND
```

Ratio = 3.3 / (2.2 + 3.3) = **0.6** → 5 V becomes 3.0 V. Tell preOBD:
`SET A3 DIVIDER 0.6` (the OM617 configs already carry `"divider": 0.6`).
Use 1 % resistors and confirm the actual ratio with a meter.

These sensors are ratiometric: their output scales with the 5 V supply. A
sagging 5 V rail reads as a pressure change. Keep the 5 V rail clean and
check it during [bench testing](../testing/BENCH_TESTING.md).

## 3. Battery / charging voltage

preOBD's `VOLTAGE_DIVIDER` sensor with custom resistors (the OM617 configs set
r1 = 47 kΩ, r2 = 10 kΩ):

```
 fused 12V (ignition) ──[47k]──┬──[1k]──┬── GPIO
                               │        │
                             [10k]    100nF + 3.3V zener/TVS to GND
                               │
                              GND
```

16 V → 2.81 V at the pin. The zener clamps load-dump spikes. Measure the
real ratio and correct with `SET <pin> CAL VOLTAGE_DIVIDER <r1> <r2> [<corr>]`.

## 4. Thermocouple (EGT)

- MAX31855K breakout on 3.3 V, SPI (SCK 12, MISO 13, CS 14 by default).
- Type-K extension wire all the way to the breakout's screw terminals. A
  copper joint anywhere makes a second thermocouple junction.
- Prefer an **ungrounded** (insulated-junction) probe. A grounded probe ties
  the thermocouple to the exhaust manifold; any ground offset between engine
  and unit then appears in the reading or trips the MAX31855's short-to-GND
  fault.
- Twist the pair; keep it away from the alternator and starter cables.
- An open probe reads as **sensor fault** (`null`) in OMOBD, never 0 °C.

## 5. RPM

### Alternator W terminal
Use the protected divider/zener circuit in preOBD's
[W-Phase RPM Guide](../guides/sensor-types/W_PHASE_RPM_GUIDE.md) **with the
3.3 V zener** (the ESP32-S3 is a 3.3 V board). The W terminal can exceed 30 V.
Not every alternator has a W terminal; diesel conversions often have one
because a tach needs it, but check.

### Magnetic (VR) crank/flywheel pickup
A VR sensor's output is AC whose amplitude rises with speed (tens of volts).
Use a dedicated VR interface IC (e.g. MAX9926 class) that outputs a clean
logic pulse, powered at 3.3 V or followed by a level shifter. Do not use a
plain divider.

### Hall-effect pickup or factory tach signal
Open-collector hall sensors: pull-up to 3.3 V (not 5 V) through 4.7–10 kΩ.
Unknown factory signals: **measure with an oscilloscope first** (amplitude,
polarity, pulses per revolution). If the signal belongs to a control unit
(later OM60x engines), use a high-impedance buffer so the control unit sees
no load, or don't tap it.

## 6. Digital inputs (switches, float switches)

Switch to ground, internal pull-up, 1 kΩ series + 100 nF. Never switch 12 V
into a GPIO; use an optocoupler or a divider + clamp.

## 7. Outputs

preOBD supports relay outputs (control-engine Actions). **OMOBD's engine
profiles define none**, and none should ever be wired to fuel, starting,
shutdown or anything else that affects how the engine runs.
