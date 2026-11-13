# Alarm thresholds

**OMOBD ships no engine warning limits.** Every threshold slot in the engine
profiles is `null`, and the profile configs import with `"rules": []`, so
nothing alarms until you decide it should.

Why: OM engines differ by variant, turbo, injection-pump setting, use and
condition, and this project has not yet verified factory figures. A wrong
universal limit either never warns or warns all the time, and both teach the
driver to ignore it.

## Watch out for preOBD's generic defaults

Typing `SET <pin> <APPLICATION> <SENSOR>` at the CLI **seeds preOBD's generic
default alarm** for that application (for example EGT above 600 °C, oil
pressure below 1.0 bar). Those are generic preOBD values, **not OM617
specifications**; an OM617's hot-idle oil pressure, for instance, may sit
below a generic 1.0 bar floor. After using `SET` to add an input, review
`RULE LIST` and remove or change any rule you haven't validated:

```
RULE LIST
SET A2 ALARM DISABLE
```

## Where to get limits

1. The factory service manual for your engine variant (oil pressure at idle
   and at speed, thermostat opening, coolant system).
2. The turbocharger/injection setup you actually have (boost).
3. Your own baseline: log a healthy engine across conditions first; set
   warnings outside what normal looks like.
4. For EGT, choose and document your own limit and its source; probe
   location changes readings (pre-turbo reads hotter than post-turbo).

Record each limit and its source in your profile copy (`thresholds.*.value`,
`status: "validated"`, `source`), so the validator accepts it and the next
person can see where it came from.

## Setting them

preOBD alarm rules (base units: °C, bar, V). Examples with placeholder values:

```
CONFIG
SET 14 ALARM MAX 650
RULE ADD A0 ABOVE 105 WARNING
SAVE
RUN
```

Rules have warm-up and debounce; see preOBD's
[Alarm System Guide](../guides/configuration/ALARM_SYSTEM_GUIDE.md).

## Oil pressure vs RPM (OMOBD)

A single "low oil pressure" floor is wrong at either idle or speed. OMOBD's
advisory compares oil pressure with a curve you enter (up to 6 points),
after a warm-up, with debounce:

```json
"oilCurve": { "points": [ {"rpm": 750, "minBar": 0.0}, {"rpm": 3000, "minBar": 0.0} ],
              "warmupMs": 10000, "debounceMs": 2000 }
```

Replace the zeros with figures from your service manual. The curve ships
empty and stays off until you fill it. It needs the RPM and oil-pressure
inputs set in `pins`. It drives a dashboard banner only; it switches nothing.
