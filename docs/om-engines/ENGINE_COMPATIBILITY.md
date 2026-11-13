# OM engine compatibility matrix

| Engine | Layout | Aspiration | Injection (to verify per variant) | Factory ECU | OMOBD profile | Sensor configs | Vehicle-tested |
|---|---|---|---|---|---|---|---|
| **OM617** | inline-5, 3.0 L | NA (OM617.912) and turbo (OM617.95x) | mechanical inline pump | none | **development** | ✅ ESP32-S3, NA + turbo | ❌ |
| OM615 | inline-4 | NA | mechanical | none | planned | — | ❌ |
| OM616 | inline-4 | NA | mechanical | none | planned | — | ❌ |
| OM601 | inline-4 | NA | mechanical | none | planned | — | ❌ |
| OM602 | inline-5 | NA and turbo | mechanical; some variants add electronics | varies | planned | — | ❌ |
| OM603 | inline-6 | NA and turbo | varies | varies | planned | — | ❌ |
| OM604 | inline-4, 4-valve | NA | varies | varies | planned | — | ❌ |
| OM605 | inline-5, 4-valve | NA and turbo | varies | varies | planned | — | ❌ |
| OM606 | inline-6, 4-valve | NA and turbo | varies; turbo variants commonly electronic | varies | planned | — | ❌ |

**Every identification column is unverified** (general reference
knowledge, not checked against Mercedes-Benz documentation in this
repository). Each profile carries `"verified": false` until someone adds a
source. Use the number stamped on your engine block and your service manual.

## What "profile status" means

| Status | Meaning |
|---|---|
| development | Being actively built and tested; has importable sensor configs |
| planned | Profile skeleton: capabilities OMOBD's firmware *could* measure, no engine-specific data |
| supported | Validated on at least one running engine, with records (none yet) |

Capabilities inside a profile are `tested`, `proposed`, `unverified` or
`unsupported`. `tested` requires a `testedOn` record; the validator
(`tools/omobd/validate_profiles.py`) refuses it otherwise.

## Mechanical vs electronically controlled engines

- **Mechanical (OM615/616/617/601 and early 602/603):** no control unit. Every
  OMOBD sensor is an addition. Nothing OMOBD does can affect running.
- **Electronically assisted or controlled (some OM602/603, OM604/605/606):**
  control units may own the speed, temperature or boost sensors. Rules:
  - Never connect OMOBD in parallel with a control unit's sensor without
    knowing the circuit: an extra load or ground offset changes what the
    control unit sees.
  - Prefer separate OMOBD sensors, or high-impedance buffered taps.
  - OMOBD does not read or emulate those control units' diagnostics.

## Adding an engine

1. Copy a planned profile, e.g. `engine-profiles/OM602/OM602.profile.json`.
2. Fill `identification` with sourced facts and set `verified` only with a `source`.
3. Add a variant with `config`/`settings` files (see the OM617 folder).
4. Run `python3 tools/omobd/validate_profiles.py` and `pio test -e native`.

No core monitoring code changes are needed: profiles are data.
