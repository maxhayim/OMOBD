# Contributing to OMOBD

Thanks for helping. OMOBD is a non-commercial fork of preOBD for
Mercedes-Benz OM diesels; contributions are accepted under the same
MIT + NonCommercial terms ([docs/LICENSING.md](docs/LICENSING.md)).

## Most wanted

1. **Installation reports** — vehicle, engine variant (stamped number),
   sensors, wiring, calibration log, what worked and what didn't
   ([vehicle testing](docs/testing/VEHICLE_TESTING.md)).
2. **Sourced engine data** — factory specifications with the document they
   came from (manual title, section, page). Unsourced numbers can't be
   accepted into profiles.
3. **EDMS-II research** — work through the PeachParts thread using the
   [checklist](docs/history/EDMS-II.md#6-research-checklist-to-complete-from-the-thread);
   link every fact to its post. Don't upload Paul's images or files without
   his written permission.
4. Bench results on ESP32-S3 boards and phones/browsers.

## Ground rules

- **Monitoring only.** No code or wiring that controls fuel, starting,
  shutdown or anything else affecting how the engine runs.
- **Say what was tested.** "Builds" and "host tests pass" are not "works on
  the engine". Mark profile capabilities `tested` only with a `testedOn` record.
- **No invented data.** Calculated values are labelled calculated; missing
  sensors produce no data; there are no fake fault codes.
- **No universal thresholds.** Limits are per engine, sourced, and validated.
- **Keep upstream changes small** and send general fixes to
  [preOBD](https://github.com/preobd/preOBD) too.

## Before opening a pull request

```bash
pio test -e native
pio run -e esp32s3_omobd
python3 tools/omobd/validate_profiles.py
python3 -m unittest discover -s tools/omobd
python3 tools/check_layering.py
python3 tools/check_docs.py
```

Enable the pre-commit hook once: `git config core.hooksPath .githooks`.
See the [development guide](docs/omobd/DEVELOPMENT.md).

## Adding an engine profile

See [Engine compatibility](docs/om-engines/ENGINE_COMPATIBILITY.md#adding-an-engine)
and [engine-profiles/README.md](engine-profiles/README.md).

## Reporting bugs

Include `VERSION`, `SYSTEM DUMP JSON`, the dashboard's Unit status, board,
engine/variant, and steps to reproduce.
