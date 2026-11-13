# Vehicle testing

Only after [bench testing](BENCH_TESTING.md). Keep mechanical gauges as the
reference throughout. Nobody watches the dashboard while driving: a
passenger reads it, or rely on logs.

## Stage 1 — stationary, engine off

- Key on: unit starts, network appears, dashboard loads.
- Every temperature ≈ ambient (cold engine). MAP = atmosphere; boost ≈ 0.
- Battery voltage matches a meter.
- Factory gauges and warning lights behave exactly as before OMOBD.

## Stage 2 — stationary, engine running

- RPM at idle matches a reference tach ([calibration](../calibration/RPM_CALIBRATION.md)).
- Oil pressure rises immediately after start and matches the mechanical gauge.
- Charging voltage rises; no warnings about voltage on the factory cluster.
- Warm-up: coolant and oil temperatures climb smoothly; compare coolant to
  the factory gauge at thermostat-open.
- Vacuum reaches its normal level; vacuum-operated functions still work.
- EGT at idle is stable and plausible; blip the throttle and watch it respond.
- **Stop the engine with the key** as usual: confirm OMOBD has no effect on
  shutdown.
- Check every new fitting for leaks (oil, vacuum, boost, exhaust at the EGT bung).

## Stage 3 — driving (passenger or logging)

- Steady cruise, then a sustained climb or load: boost and EGT respond, values
  are steady, no dropouts.
- Note any reading that jumps when lights, wipers, blower or glow plugs switch
  (grounding/noise).
- Rough roads: watch for intermittent sensor faults (connector or wiring).

## Stage 4 — logs and records

- With an SD module, log a drive (`OUTPUT SD_Log ENABLE`) and review it.
- Fill in [HARDWARE_VALIDATION_STATUS.md](HARDWARE_VALIDATION_STATUS.md) and,
  per capability you checked, add a `testedOn` record in your profile copy:

```json
"testedOn": [{ "date": "YYYY-MM-DD", "vehicle": "…", "engine": "OM617.952",
               "reference": "mechanical oil gauge", "result": "within 0.1 bar idle and 2500 rpm" }]
```
