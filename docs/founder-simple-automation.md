# Founder simple automation

Scope: device assignment plus one operating mode per device. No multi-day phases,
grow recipes or application-specific crop modes. Frozen V1 code is unchanged.

## User flow and capabilities

Activate a fixed port, choose its supported device, name it, then select Manual,
Time-controlled or Sensor-controlled. PWM and 0–10 V devices support percentages;
DC switch ports support on/off. Pump/monostable-valve profiles always use bounded
pulses, never continuous demand. Native PWM ports and analog/switched fan profiles
independently select circulation, intake or exhaust. Role is metadata, not a hard-
coded connector behavior or an assumption that outside air cools/dries a room.

Time control supports up to eight daily windows/pulses or a repeating daily
interval (1..1440 minutes) anchored to a local start time. Continuous interval
runtime is 1..period minutes; pulse duration is 1..device-limit milliseconds.
Intervals recur from the anchor through the local calendar day, reset daily,
and may carry the last continuous window over midnight. They are not a monotonic
24/7 stopwatch when the period does not divide a day. Europe/Berlin time and
existing durable pulse high-water marks handle clock reversal/repeated DST hours;
only the latest eligible missed pulse can be caught up, never a queue.

## Sensor rules

Up to six ordered rules per device. Every choice binds the exact device source
(type/bus/address) and a named measurement, including multiple sensors of the
same type/address on different mux channels. Supported current measurements are
temperature, humidity, pressure, CO2, lux and UV, as supplied by detected devices.
Disconnected selections remain saved and visible. The global best-source registry
continues to support old telemetry, but cannot silently replace an automation input.

Each rule has a target, an explicit device effect (raise/lower), proportional band,
hysteresis and debounce (default 5 s, configurable 0..300). Target control is bounded
proportional demand, not PID or a guarantee of simultaneous environmental targets.
Normal compatible demands combine by maximum; ties retain the earlier rule.
For switches/pulse triggers the demand is 0 or 100. Optional minimum < target and
maximum > target generate warnings after debounce with hysteretic clearing.
Each limit may warn only or demand a specific output. The first active limit action
in priority order overrides all normal target demands and lower limit actions.
Runtime status includes winning rule, reason, demand and a per-rule warning mask.

Any missing, invalid, nonfinite or >=12-second-old selected input immediately uses
the device fallback (0..100 for analog, 0/100 for switches). All rule state resets
on loss. Pulse devices always fall back to OFF. Recovery uses the configured
normal debounce again. Manual/OTA/Setup/Pause/driver/storage/time interlocks take
precedence over every configured action. No external controller writes GPIOs;
automation supplies demands through the existing device/driver boundary.

## Persistence and watering

The existing revisioned NVS configuration persists mode, role, rules, limits,
fallback and timing. Legacy missing fields migrate to manual/no sensor rules;
legacy enabled plans are presented as time-controlled. Previous pending fan
automation with no rules remains OFF. Inactive saved rules/plans survive mode
switches. Lowering a pulse protection limit also caps dormant pulse settings;
active settings that exceed the limit are rejected. Input/configuration JSON is
bounded to 12 KiB; large temporary configurations live on the heap, not the small
ESP task stack. Writes fail before changing assignments.

Time and sensor modes use the existing checksummed explicit operation intent and
persistent Pause contract. After reboot they resume only after valid time and
storage/drivers/interlocks permit. One-off manual demands never resume. Evaluator
updates do not write NVS or clear PWM tacho faults.

Sensor watering: a positive demand starts one bounded impulse, then a selectable
minimum pause (60..86400 s) applies. If demand remains, another impulse may follow
after that pause. The existing checksummed last-run journal durably reserves the
occurrence BEFORE GPIO, records accepted start and observed completion/interruption,
and blocks duplicate/future high-water tokens. Cold reserved/started records become
INTERRUPTED_UNKNOWN and are never replayed immediately. The recorded time never
moves backward on boot/completion, so a clock rollback cannot shorten the pause.
Absent demand stops an active sensor pulse; journal failure stops outputs and
persists Pause. Physical timer cutoff, 10-second profile cap, lower per-device cap
and 10-minute pump boot lock remain active. Water delivery remains unmeasured.

## Validation

`tests/founder/run_tests.py` includes pure-rule priority/hysteresis/debounce/fallback/
clock-wrap tests, interval boundaries, exact-source driver tests, sensor device
integration and fresh-process pulse journal restoration. Existing lifecycle,
Wi-Fi setup and device/storage fault tests remain. Browser scripts cover fixed
ports, old schedules/fans, and `check_automation_ui.cjs` exercises source selection,
limits/actions/order, client validation, reload, absent sensor retention, roles,
intervals, protected pump configuration and mobile layout. Browser tests use
fixtures, not commands to real outputs. Actual sensor/control response and loads
still require physical acceptance. Installed source/hash are recorded separately
in founder-first-start.md and the internal software-validation.md.

Sensor selector options refresh when devices disconnect/reconnect without a page reload or configuration revision. Existing selections, targets and unsaved rule drafts remain unchanged; missing bindings are clearly labelled.
