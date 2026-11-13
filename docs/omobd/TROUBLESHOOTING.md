# OMOBD troubleshooting

For sensor, CLI and output problems see preOBD's
[Troubleshooting](../TROUBLESHOOTING.md). This page covers what OMOBD adds.

| Symptom | Likely cause | What to do |
|---|---|---|
| No `OMOBD-xxxx` network | Wi-Fi mode `OFF` or `STA`; AP failed to start | USB console shows the mode and errors. In `STA` mode the AP appears only after 20 s without a connection. |
| Don't know the Wi-Fi password | Generated on first boot | Read it from the USB console at start-up (`pio device monitor`) |
| Dashboard says "connection lost" | Phone left the unit's network (many phones drop Wi-Fi without internet) | Turn off "switch to mobile data"/"auto-join better network" for this network; stay on the page |
| Settings won't save: "Wrong or missing Wi-Fi password" | HTTP changes need the unit password | Enter it in Device settings |
| Settings rejected with a reason | Validation (e.g. `fixedBar` outside 0.5–1.1, oil curve not ascending, curve without pins) | Fix the field named in the message |
| Unit status says settings are "defaults (…)" | Stored settings were missing or invalid at boot | Re-enter or restore your settings backup |
| Boost reads ≈ −1 bar | MAP input not configured, or `pins.boostMap` points at the wrong input | Check `pins` against `LIST INPUTS` |
| Boost reads high/low by a constant at rest | Atmospheric reference wrong (altitude) | Set `fixedBar` to local absolute pressure, or use `INPUT`/`KEY_ON_MAP` |
| Vacuum/boost show `—` | Input fault, or reference invalid | Sensors page shows which |
| Tile says "Not fitted" | The profile layout includes a gauge for an input you don't have | Remove the gauge (Edit layout) or configure the input |
| Tile says "Out of range" | Reading beyond the sensor's range: wrong sensor type, divider ratio, open/short wire | Check wiring, `SET <pin> DIVIDER`, and the sensor name |
| Engine hours don't advance | No RPM input in `pins.rpm`, or RPM below `runningRpm` | Configure RPM; check calibration |
| Oil advisory always "unconfigured" | Curve is empty by design | Enter validated points ([Alarm thresholds](../calibration/ALARM_THRESHOLDS.md)) |
| Alarms you didn't set | Added inputs with `SET`, which seeds preOBD's generic defaults | `RULE LIST`, then remove/adjust |
| JSON IMPORT refuses an OM617 config | Board isn't an ESP32-S3 (pin numbers differ) | Edit the pins for your board, or use the ESP32-S3 |
| BLE OBD app can't find the unit | It's named **OMOBD** on this build | Search for OMOBD; see [Virtual OBD](VIRTUAL_OBD.md) |

When asking for help include: `VERSION`, `SYSTEM DUMP JSON`, the dashboard's
Unit status table, your profile/variant, and what you've tried.
