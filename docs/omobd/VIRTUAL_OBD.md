# Virtual OBD in OMOBD

An OM617 has **no engine control unit and no OBD system**. OMOBD keeps
preOBD's virtual OBD-II interface so standard apps can display its sensor
data, and is explicit about what that data is.

## What OMOBD reports, by kind

| Kind | Examples | Over OBD (Mode 01) | On the web dashboard |
|---|---|---|---|
| **Real measurement** | EGT, oil pressure, coolant temp, MAP, RPM | yes — standard PID where J1979 has one (0x05 coolant, 0x0B MAP, 0x0C RPM, 0x78 EGT…), preOBD custom PIDs 0xAA–0xB9 otherwise | value, `src: measured` |
| **Imported** | values read from another controller over CAN | yes, as configured | `src: imported`, tagged CAN |
| **Calculated** | boost (MAP − atmosphere), vacuum depth, engine hours, oil-vs-RPM state | **no** (dashboard only) | value, `src: calculated`, tagged CALC |
| **Synthetic OBD framing** | supported-PID bitmaps, PID 0x01 monitor status | yes — required by the protocol; reports MIL off and no readiness monitors | — |
| **Not supported** | ECU fault codes, freeze frames, readiness monitors, VIN | DTC requests (modes 03/07/0A) answer **"0 codes"**; nothing is invented | — |

A missing or faulted sensor is never encoded: preOBD's encoder returns
nothing for a non-finite value, so an app sees "no data" rather than 0
(pinned by `test_omobd_presets` and upstream's `test_obd_encode`).

Boost via OBD: apps that compute boost from PID 0x0B (MAP) and their own
barometric value will show boost; OMOBD's own calculated boost is a dashboard
value.

## New OMOBD PIDs

| PID | Application | Formula (Torque syntax) | Unit |
|---|---|---|---|
| `0xB8` | `VACUUM` | `((A*256)+B)/10` | kPa absolute (vacuum depth = atmosphere − value) |
| `0xB9` | `ENGINE_BAY_TEMP` | `((A*256)+B)/10-40` | °C |

Full table: [OBD2 PID reference](../reference/OBD2_PID_REFERENCE.md).

## App compatibility (assessment from upstream capabilities)

Not yet tried with OMOBD hardware. preOBD documents:

| App | Link | Basis |
|---|---|---|
| OBD Fusion (iOS/Android) | BLE ELM327 emulation, or CAN + ELM327 adapter | [Direct BLE OBD guide](../guides/outputs/DIRECT_BLE_OBD_GUIDE.md) |
| Car Scanner | same | same |
| Torque Pro (Android) | BLE, or CAN + adapter. Torque Lite: Classic SPP only, so CAN + adapter | same; [OBD-II scanner guide](../guides/outputs/OBD2_SCANNER_GUIDE.md) |
| RealDash | preOBD's RealDash stream (serial/BLE) | [RealDash guide](../guides/outputs/REALDASH_SETUP_GUIDE.md); `docs/realdash/preOBD.xml` |

Custom PIDs (0xAA–0xB9) need a one-time custom-PID entry in each app.
On the ESP32-S3 the BLE link and the Wi-Fi dashboard run together; that
coexistence is on the [hardware validation list](../testing/HARDWARE_VALIDATION_STATUS.md).

The BLE device name is **OMOBD** on the `esp32s3_omobd` build.
