# OMOBD hardware requirements

This is the hardware for the `esp32s3_omobd` firmware and the OM617 profile
configs. None of it has been validated on a vehicle yet
([status](../testing/HARDWARE_VALIDATION_STATUS.md)).

## Controller

| Item | Recommendation | Why |
|---|---|---|
| Board | **ESP32-S3-DevKitC-1 N8R8** (8 MB flash, 8 MB PSRAM) or an equivalent ESP32-S3 module board | Wi-Fi + BLE; preOBD's `esp32s3` board definition is written for it (GPIO 26–37 reserved for flash/PSRAM) |
| Alternatives | Other ESP32-S3 boards with ≥ 8 MB flash | Check which GPIOs the module uses for flash/PSRAM and adjust |
| Not for the web dashboard | Teensy, STM32, Mega | preOBD still runs on them (no Wi-Fi dashboard) |

**ADC:** only **A0–A9 (GPIO 1–10, ADC1)** can be used for analog inputs. ADC2
stops working while the radio is on, and OMOBD always has the radio on.
Analog inputs accept **0–3.3 V maximum**; anything else needs a divider or
conditioning circuit.

## Power

| Item | Recommendation |
|---|---|
| Source | Ignition-switched (accessory/run) 12 V circuit, so the unit cannot flatten the battery |
| Fuse | Inline fuse at the tap, **2 A** or less, as close to the source as possible |
| Regulator | Automotive-rated 12 V → 5 V buck converter, input rated ≥ 40 V, with reverse-polarity protection (series diode or ideal-diode/P-FET) and a TVS (e.g. SMBJ-series, ~24 V standoff) across the input for load-dump and alternator spikes |
| Board feed | 5 V into the board's 5 V/VIN pin (the board's own 3.3 V regulator then powers the ESP32-S3) |
| Sensor supply | 5 V sensors (MPX4250AP, 0.5–4.5 V transducers) from the same 5 V rail; one star point |
| Filtering | 100 µF electrolytic + 100 nF ceramic at the board's 5 V input |

Never power the board straight from 12 V or from a cigarette-lighter USB
adapter that isn't rated for automotive transients for a permanent install.

## Grounding

- One **ground reference**: run the unit's ground to the **engine block** or
  the main ground point the gauges use, not to a random body panel.
- Sensor grounds (MAP sensors, thermistor bias network) return to the unit's
  ground star, not separately to the chassis.
- Resistive senders that ground through their thread (VDO, Bosch NTC) use the
  engine block as their return: the unit's ground **must** be on the same
  block, or the reading will include the voltage drop between grounds.
- On an engine conversion (e.g. OM617 in a Jeep) check that the engine has a
  heavy ground strap to the chassis **and** to the battery negative.

## Enclosure and wiring

- Mount inside the cabin, away from heater ducts; under-hood temperatures are
  beyond what a hobby board and its connectors are rated for.
- Automotive wire (TXL/GXL), sealed connectors in the engine bay, grommets
  through the firewall.
- Shielded twisted pair for the RPM signal; thermocouple **extension wire**
  (type K, not copper) from the probe to the MAX31855.
- Route away from glow-plug wiring, the starter cable and the alternator output.

## Interfaces used by the OM617 configs

| Function | GPIO | Interface | Conditioning |
|---|---|---|---|
| Coolant temperature | 1 (A0) | ADC1 | bias resistor ([preOBD guide](../guides/hardware/BIAS_RESISTOR_GUIDE.md)) |
| Oil temperature | 2 (A1) | ADC1 | bias resistor |
| Oil pressure (resistive sender) | 3 (A2) | ADC1 | bias resistor |
| Manifold absolute pressure (boost) | 4 (A3) | ADC1 | 2.2 kΩ / 3.3 kΩ divider (ratio 0.6) |
| Vacuum line pressure | 5 (A4) | ADC1 | 2.2 kΩ / 3.3 kΩ divider |
| Battery voltage | 6 (A5) | ADC1 | 47 kΩ / 10 kΩ divider + clamp |
| Transmission temp (optional) | 7 (A6) | ADC1 | bias resistor |
| Intake air temp (optional) | 8 (A7) | ADC1 | bias resistor |
| Fuel supply pressure (optional) | 9 (A8) | ADC1 | 2.2 kΩ / 3.3 kΩ divider |
| Engine bay temp (optional) | 10 (A9) | ADC1 | bias resistor |
| EGT (MAX31855 CS) | 14 | SPI (SCK 12, MISO 13, MOSI 11) | MAX31855 breakout, 3.3 V |
| RPM | 15 | interrupt | W-terminal / pickup conditioning |
| SD card (optional) | set `SD_PIN` in `boards/esp32s3_omobd/wiring.ini` | SPI | module with 3.3 V logic |
| CAN (optional) | 20 TX / 21 RX | TWAI | 3.3 V CAN transceiver (SN65HVD230 class) |

Exact circuits: [SIGNAL_CONDITIONING.md](SIGNAL_CONDITIONING.md).
Sensor choices: [SENSOR_COMPATIBILITY.md](SENSOR_COMPATIBILITY.md).

## Bill of materials (OM617 turbo, full set)

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32-S3-DevKitC-1 N8R8 | |
| 1 | Automotive 12→5 V buck module (≥ 1 A) + TVS + reverse protection | |
| 1 | Inline fuse holder + 2 A fuse | |
| 1 | MAX31855K breakout | K-type only |
| 1 | K-type EGT probe, ungrounded (insulated) junction, with compression fitting | 1/8" NPT is common; check the manifold boss/bung |
| — | Type-K thermocouple extension wire | |
| 2 | MPX4250AP (or 0.5–4.5 V absolute MAP sensor with a known curve) | boost and vacuum |
| 1 | VDO 120 °C temperature sender + adapter | coolant |
| 1 | VDO 150 °C temperature sender + oil adapter | oil temperature |
| 1 | VDO 5 bar pressure sender + tee | oil pressure |
| — | Resistors (1 %, metal film), 3.3 V zener or TVS diodes, 100 nF capacitors | conditioning |
| 1 | RPM conditioning parts per [preOBD W-phase guide](../guides/sensor-types/W_PHASE_RPM_GUIDE.md) | |
| opt. | MicroSD module, CAN transceiver, BME280 | logging, CAN, barometric reference |
