# OMOBD development guide

OMOBD is preOBD plus a thin layer. Read preOBD's
[development docs](../development/README.md) first; everything there applies.

## Layout

| Path | What |
|---|---|
| `src/omobd/omobd_derived.*` | Pure logic: pressure maths, input health, oil-vs-RPM monitor, hour meter |
| `src/omobd/omobd_settings.*` | Pure logic: settings struct, validation, JSON codec |
| `src/omobd/omobd_live.*` | Pure logic: `/api/live` document writer |
| `src/omobd/omobd_history.h` | Fixed-size ring buffer |
| `src/omobd/omobd_web.*` | ESP32 glue: Wi-Fi, HTTP server, NVS, wiring the above to preOBD |
| `src/omobd/dashboard_html.h` | Generated: gzipped dashboard + profile catalog |
| `dashboard/index.html` | Dashboard source (plain HTML/CSS/JS, no build tools) |
| `engine-profiles/` | Engine profiles and per-board configs |
| `boards/esp32s3_omobd/` | Board definition (`OMOBD_WEB_DASHBOARD`, `PRODUCT_NAME`), pinned platform |
| `scripts/omobd_embed_dashboard.py` | Builds `dashboard_html.h` (runs automatically in the OMOBD build) |
| `tools/omobd/` | Profile validator and its tests |
| `test/test_omobd_*` | Host tests |

The pure modules compile only for OMOBD builds and host tests (guarded by
`OMOBD_WEB_DASHBOARD || NATIVE_TEST`), so other preOBD boards produce the
same firmware as upstream. Edits to upstream files are kept small:
`src/main.cpp` (three call sites, product name), two appended application
presets with their PIDs, the IDF 5 watchdog fix, docs.

## Build and test

```bash
pio run -e esp32s3_omobd                 # firmware
pio run -e esp32s3_omobd -t upload       # flash
pio test -e native                       # all host tests (preOBD + OMOBD), macOS
pio test -e native -f "test_omobd_*"     # OMOBD only
python3 tools/omobd/validate_profiles.py
python3 -m unittest discover -s tools/omobd
python3 tools/check_layering.py
python3 tools/check_docs.py
```

Use a PlatformIO build directory outside cloud-synced folders if builds are
slow (`export PLATFORMIO_WORKSPACE_DIR=~/.cache/omobd-pio`).

Enable preOBD's pre-commit hook in a fresh clone:
`git config core.hooksPath .githooks`.

## Working on the dashboard

Edit `dashboard/index.html`, then `python3 scripts/omobd_embed_dashboard.py`
(the firmware build also runs it). CI fails if the committed header is stale.
For quick UI work you can serve the file locally and point it at a unit, or
stub `/api/live` with a static JSON file.

## Staying in sync with preOBD

```bash
git fetch upstream
git merge upstream/main
pio test -e native && pio run -e esp32s3_omobd
```

Changes useful to preOBD in general (fixes, new sensors) should go upstream
as pull requests rather than living only here.
