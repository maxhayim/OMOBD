# Bench testing

Do all of this on the bench before anything is fitted to a vehicle. Use a
current-limited bench supply (set ~13.8 V, 0.5 A limit) in place of the car.

## 1. Firmware and network

1. Flash: `pio run -e esp32s3_omobd -t upload`, then `pio device monitor`.
2. The console prints the Wi-Fi network (`OMOBD-xxxx`), the password, and
   `http://192.168.4.1`. Join it from a phone; the dashboard loads.
3. `VERSION`, `SYSTEM DIAG` on the console: no errors.
4. Unplug USB and power from the 12 V supply through your converter; the
   network and dashboard come back without USB.

## 2. Each input, one at a time

| Input | Bench stimulus | Expected |
|---|---|---|
| Resistive temperature sender | decade box or fixed resistors at a few points of the sender's table; or the sender in ice water / boiling water | readings follow the table |
| Resistive pressure sender | decade box at the table's points | readings follow the table |
| MAP / 0.5–4.5 V sensor | sensor on a hand vacuum/pressure pump with a reference gauge | MAP = local atmosphere at rest; tracks the pump |
| Battery divider | vary the bench supply 11–15 V | within ~0.1 V of a meter after calibration |
| EGT | room temperature, then a heat gun or lighter on the probe | rises with heat; polarity right |
| RPM | signal generator (square wave, 3.3 V, 0–1 kHz) into the **conditioned** input, or the conditioning circuit driven by the generator at higher amplitude | RPM = expected value for the frequency and calibration |

Use preOBD's test mode for logic without sensors (see `src/test/README.md`).

## 3. Fault behaviour (the reliability checks)

| Action | Expected on the dashboard |
|---|---|
| Unplug the thermocouple | EGT tile: "Sensor fault", value `—` (never 0) |
| Open a temperature sender wire | "Out of range" or "Sensor fault", not a plausible number |
| Short a 0.5–4.5 V sensor output to ground | "Out of range" |
| Turn Wi-Fi off on the phone for 30 s, then back on | "Connection lost" banner while away; recovers by itself |
| Close the browser for 5 minutes | Unit keeps logging/alarming; reopening shows current values |
| Power-cycle the unit 10× quickly | Starts every time; settings intact or reported as reset to defaults |
| Save a settings JSON with a bad value (e.g. `fixedBar: 5`) | Refused with a reason; old settings stay |

## 4. Endurance

Run for at least 24 h with sensors connected, dashboard open on one device
and closed on another. Afterwards check **Settings → Unit status**: uptime
continuous (no unexpected reset), lowest free memory stable.

Record results in [HARDWARE_VALIDATION_STATUS.md](HARDWARE_VALIDATION_STATUS.md).
