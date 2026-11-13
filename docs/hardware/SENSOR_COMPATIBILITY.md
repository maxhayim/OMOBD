# Sensor compatibility matrix

What OMOBD (through preOBD's sensor library) can read, and what's been
checked. **"Firmware"** = a driver and calibration exist and the host tests
cover the maths. **"Vehicle"** = verified on a running OM engine against a
reference instrument. No row has vehicle verification yet.

| Measurement | preOBD application | Sensor name(s) in firmware | Interface | Firmware | Vehicle | Notes |
|---|---|---|---|---|---|---|
| EGT | `EGT` | `MAX31855`, `MAX6675` | SPI | ✅ | ❌ | MAX31855 recommended (wider range, fault bits) |
| Boost (calculated) | `MAP_PRESSURE` + OMOBD derived | `MPX4250AP`, `MPX5700AP` | ADC + divider | ✅ | ❌ | Absolute sensor; boost = MAP − atmosphere |
| Boost (gauge sensor) | `BOOST_PRESSURE` | `GENERIC_BOOST`, `VDO_2BAR_*` | ADC | ✅ | ❌ | Alternative to MAP + reference |
| Vacuum | `VACUUM` (OMOBD) + derived | `MPX4250AP` | ADC + divider | ✅ | ❌ | Low resolution near 1 bar; a 15–115 kPa part is on the roadmap |
| Coolant temperature | `COOLANT_TEMP` | `VDO_120C_TABLE`, `VDO_120C_STEINHART`, `BOSCH_NTC_M12`, `GENERIC_NTC_*` | ADC + bias | ✅ | ❌ | Separate immersed sender |
| Oil temperature | `OIL_TEMP` | `VDO_150C_TABLE`, `VDO_150C_STEINHART`, `GENERIC_NTC_*` | ADC + bias | ✅ | ❌ | |
| Transmission temperature | `TRANS_TEMP` | `VDO_150C_TABLE`, `GENERIC_NTC_*` | ADC + bias | ✅ | ❌ | |
| Intake air temperature | `INTAKE_AIR_TEMP` | `BOSCH_NTC_M12`, `GENERIC_NTC_*` | ADC + bias | ✅ | ❌ | |
| Engine compartment temp | `ENGINE_BAY_TEMP` (OMOBD) | `GENERIC_NTC_BETA`, `GENERIC_TEMP_LINEAR` | ADC | ✅ | ❌ | |
| Oil pressure | `OIL_PRESSURE` | `VDO_5BAR_TABLE`, `VDO_10BAR_TABLE`, `GENERIC_PRESSURE_150PSI` | ADC | ✅ | ❌ | RPM-dependent advisory in OMOBD |
| Fuel supply pressure | `FUEL_PRESSURE` | `GENERIC_BOOST`, `GENERIC_PRESSURE_150PSI` | ADC + divider | ✅ | ❌ | Low-pressure side only |
| Engine RPM | `ENGINE_RPM` | `W_PHASE_RPM` | interrupt | ✅ | ❌ | Conditioning required |
| Vehicle speed | `VEHICLE_SPEED` | `HALL_SPEED` | interrupt | ✅ | ❌ | |
| Battery / charging voltage | `PRIMARY_BATTERY` | `VOLTAGE_DIVIDER` | ADC + divider | ✅ | ❌ | |
| Barometric reference | `BAROMETRIC_PRESSURE` | `BME280_PRESSURE` | I2C | ✅ | ❌ | Check I2C pins on ESP32-S3 (see below) |
| DS18B20 temperatures | — | — | 1-Wire | ❌ | ❌ | Not in preOBD; planned ([Roadmap](../../ROADMAP.md)) |
| Fuel flow / consumption | — | — | — | ❌ | ❌ | Needs supply **and** return flow; any figure would be an estimate |
| CAN-imported values | any | `CAN_IMPORT` | TWAI + transceiver | ✅ | ❌ | For engines/vehicles that have a CAN source |

## Immersed vs surface-mounted temperature sensors

A sender **immersed** in the fluid (coolant passage, oil gallery, sump) reads
the fluid. A sensor **clamped or taped to the outside** of a pipe, housing or
sump reads the metal surface, which:

- reads **lower** than the fluid, by an amount that depends on airflow,
  speed, ambient temperature and insulation;
- **lags** behind fluid changes (seconds to minutes);
- can be fooled by radiant heat from the exhaust.

Surface readings are useful for trends and comparisons, not for absolute
limits. If you use one, label it (e.g. `SET A1 NAME Oil sump surface`) and do
not set alarm limits copied from immersed-sender figures.

## ESP32-S3 I2C pin note

preOBD's default I2C pins for the ESP32 family are GPIO 21/22. The ESP32-S3
has no GPIO 22, and GPIO 21 is also preOBD's default CAN RX on the S3. Before
fitting a BME280, select working pins (`BUS I2C`, see preOBD's
[Serial Commands](../reference/SERIAL_COMMANDS.md)) and check them on the
bench. This needs confirming on hardware and reporting upstream.
