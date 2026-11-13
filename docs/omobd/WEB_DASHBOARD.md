# Web dashboard and HTTP API

The `esp32s3_omobd` firmware serves a dashboard and a small JSON API from
the unit. Nothing is fetched from the internet.

## Connecting

| Mode (`wifi.mode`) | What happens |
|---|---|
| `AP` (default) | The unit makes its own network `OMOBD-xxxx` (last MAC bytes). Open `http://192.168.4.1`. |
| `STA` | Joins `staSsid` (a phone hotspot, a garage network). If it can't join within 20 s it **also** starts its own AP, so you're never locked out. The IP is printed on the USB console and shown under Unit status. |
| `OFF` | No Wi-Fi (BLE and USB still work). |

**Password:** on first boot the unit generates a random 12-character password,
stores it, and prints it on the USB console at every start-up together with
the network name. Change it in Device settings (`wifi.apPass`, 8–63 chars). An
open network needs `allowOpenAp: true` and isn't recommended. The same
password (user `omobd`) is required for every change made over HTTP; reading
values is open to anyone on the network.

## Dashboard

- **Gauge pages** from the engine profile (OM617: *Engine*, *Systems*).
  *Edit layout* adds, removes, resizes (1–3 columns), reorders and changes
  type (dial, bar, number); pages can be added and deleted. Layouts live in
  the browser (localStorage) and can be downloaded/restored as a file.
- Every tile shows source tags (**CALC**, **CAN**), sensor status (no signal
  yet, sensor fault, out of range, off, not fitted), and its alarm state from
  the unit's rules (border + "▲ Warning/ALARM" text).
- A banner shows active alarms, the oil-vs-RPM advisory, or **connection
  lost** if the unit stops answering (values are then greyed, never frozen).
- **History:** line graph of the last 10 minutes kept on the unit (1 s steps by
  default), with hover read-out and a table view; gaps where readings failed.
- **Sensors:** every input with pin, application, value, status, source.
- **Settings:** engine profile, display units for calculated values, theme,
  unit status, SD logging on/off, backups, device settings (JSON).

Gauge scales are display ranges only; alarms are the unit's rules.

## API

All responses are JSON with `Cache-Control: no-store`. Values are in the
units configured on the unit; `base` is preOBD's base unit (°C, bar, V, rpm).

| Method | Path | Auth | Purpose |
|---|---|---|---|
| GET | `/` | – | Dashboard (gzip) |
| GET | `/api/live` | – | Current values (below) |
| GET | `/api/history?pin=N` or `?key=boost\|vacuum` | – | `{"intervalMs":1000,"values":[…]}` oldest→newest, `null` = no valid reading |
| GET | `/api/status` | – | Firmware, profile, network, clients, heap, uptime, logging, settings origin |
| GET | `/api/settings` (`?download=1`) | – | OMOBD settings without passwords |
| POST | `/api/settings` | Basic `omobd:<password>` | Partial or full settings JSON; validated, saved to NVS. Wi-Fi changes apply after restart |
| GET | `/api/config/export` | – | preOBD `SYSTEM DUMP JSON` as a download |
| POST | `/api/logging` | Basic | `{"enabled":true\|false}` → SD logging on/off until restart |

### `/api/live`

```json
{"seq":42,"uptimeMs":123456,"mode":"RUN","systemSeverity":0,"profile":"OM617.95x","firmware":"…",
 "channels":[{"pin":14,"key":"EGT","abbr":"EGT","label":"Exhaust Gas Temp (pre-turbo)","unit":"C",
              "value":412.25,"base":412.250,"health":"ok","src":"measured","severity":0}],
 "derived":{"atmosphere":{"bar":1.0133,"source":"FIXED","src":"calculated"},
            "boost":{"bar":0.412,"src":"calculated"},"vacuum":{"bar":null,"src":"calculated"},
            "engineHours":{"hours":1234.56,"src":"calculated"},
            "oilVsRpm":{"state":"unconfigured","minBar":null,"src":"calculated"}}}
```

`health`: `ok`, `no_reading`, `fault`, `out_of_range`, `disabled`.
`severity`: 0 none, 1 info, 2 warning, 3 alarm. `value` is `null` unless
the reading is valid. `oilVsRpm.state`: `unconfigured`, `engine_stopped`,
`settling`, `ok`, `low_pending`, `low`, `sensor_fault`.

### Settings JSON

```json
{"schema":1,"profileId":"OM617.95x",
 "wifi":{"mode":"AP","apSsid":"OMOBD-1A2B","allowOpenAp":false,"staSsid":""},
 "atmosphere":{"source":"FIXED","fixedBar":1.01325,"inputPin":null},
 "pins":{"boostMap":4,"vacuumMap":5,"rpm":15,"oilPressure":3},
 "oilCurve":{"points":[],"warmupMs":10000,"debounceMs":2000},
 "runningRpm":300,"livePollMs":500,"historyIntervalMs":1000}
```

Add `apPass`/`staPass` inside `wifi` only when changing them.
