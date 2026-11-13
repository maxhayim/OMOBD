# Engine profiles

One folder per OM engine family. A profile is **data**: adding an engine
never needs core firmware changes.

```
engine-profiles/
  OM617/
    OM617.profile.json                        engine facts, capabilities, thresholds, dashboard layout
    configs/
      esp32s3-om617-turbo.preobd.json         preOBD JSON IMPORT file (sensor wiring for ESP32-S3)
      esp32s3-om617-turbo.omobd-settings.json OMOBD settings (which inputs feed boost/vacuum/hours)
      esp32s3-om617-na.preobd.json
      esp32s3-om617-na.omobd-settings.json
  OM615/ OM616/ OM601/ ... OM606/             planned profiles (no configs yet)
```

## The three files

| File | Read by | Purpose |
|---|---|---|
| `*.profile.json` | the dashboard (embedded at build time), the validator, people | What the engine is, what can be measured, which RPM sources apply, threshold slots, default gauge pages |
| `*.preobd.json` | preOBD's `JSON IMPORT` over USB | Inputs: pin, application, sensor, units, divider, calibration. **`rules` is always `[]`.** |
| `*.omobd-settings.json` | OMOBD dashboard → Settings → Device settings | Pins feeding the calculated values, atmospheric reference, oil curve (empty) |

## Safety rules the validator enforces

- Thresholds are `null` unless `"status": "validated"` **and** a `source` is given.
- Config files contain no alarm rules unless the profile has validated thresholds.
- A capability can't be `tested` without a `testedOn` record.
- Every application, sensor and unit name must exist in the firmware.
- Naturally aspirated configs contain no boost/MAP input.
- Analog pin labels match the ESP32-S3 (A*n* = GPIO *n*+1).

```bash
python3 tools/omobd/validate_profiles.py
python3 -m unittest discover -s tools/omobd
```

The OM617 configs are also imported through preOBD's real JSON importer in a
host test (`test/test_omobd_profile_import`).

## Loading a profile onto the unit

Over USB serial:

```
CONFIG
JSON IMPORT
```

paste the `.preobd.json` file, then a line `END JSON`, then check and save:

```
LIST INPUTS
SAVE
RUN
```

Then paste the matching `.omobd-settings.json` into the dashboard's
**Settings → Device settings** and save, and pick the profile under
**Settings → Engine profile**.
