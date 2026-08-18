# Fan Control Concept

## Layered implementation

The fan path is split into fixed layers while preserving the existing public
HTTP, MQTT, display, and control interfaces:

```text
signal_registry.cpp
  stores neutral sensor signals with value, validity, source, and update time

fan_control.cpp
  reads registered signals and evaluates configured curve rules

fan_interval.cpp
  produces uptime-based circulation demands from period/run/delay settings

fan_schedule.cpp
  produces up to three local-time window demands per fan

fan_demand.cpp
  combines curve, interval, schedule, and base producers using MAXIMUM

fan_policy.cpp
  validates, versions, persists, restores, and resets each independent policy

fan_arbiter.cpp
  resolves device fault, Rest Mode, sensor safety, manual override, automatic
  demand, and minimum runtime into one final requested percentage

fan.cpp
  adapts the existing public API and connects control, arbiter, and device layers

fan_device.cpp
  executes one requested percentage with ramp, startup boost, RPM validation,
  latched stall detection, and deliberate fault acknowledgement

fan_hw_driver.cpp
  owns ESP32 PWM, tacho interrupts, RPM calculation, and immediate emergency stop

physical fan
```

The arbiter uses one explicit priority order:

```text
DEVICE_FAULT -> REST -> SENSOR_SAFETY -> MANUAL -> AUTOMATIC -> IDLE
```

Normal curve, interval, schedule, and base demands are combined with `MAXIMUM` by the
demand layer. The arbiter never reads sensors or writes PWM. Its decision
priority is exposed in Fan 1 and Fan 2 HTTP/MQTT status.

The hardware driver has no climate, grow phase, Rest Mode, MQTT, or HTTP
knowledge. The device block has no temperature or humidity knowledge. On a
confirmed RPM stall, the device block calls the driver's immediate emergency
stop and keeps the fault latched until a deliberate new fan command or reboot.

## Symmetric fan channels

Fan 1 and Fan 2 use the same `STANDARD_PWM_TACHO` device profile. There is one
shared definition for minimum/maximum output, startup boost, ramp behavior, and
stall thresholds. Both channels run as instances of the same control, arbiter,
device, status, and command code. The runtime path iterates over all configured
fan channels instead of maintaining separate Fan 1 and Fan 2 implementations.
Channel-specific compile-time flags only describe whether physical PWM and
tacho wiring is present.

Automation capabilities are shared as well. Each fan policy contains the same
curve-rule slots, interval producer, and schedule slots. The current policy data enables
temperature and humidity for Fan 1, disables them for Fan 2, and leaves both
interval producers disabled. This is configuration, not a capability
difference.

## Signal registry

Sensor drivers publish measurements to `signal_registry.cpp`. Consumers do not
need to know whether temperature came from AHT20, SHT41, SCD41, or BME/BMP. Each
registry entry contains its value, validity, physical source, unit metadata, and
last update timestamp.

Initial registered signals:

```text
climate.temperature_c
climate.humidity_pct
climate.pressure_hpa
climate.co2_ppm
light.lux
light.uv_index
```

The fan curve engine currently consumes temperature and humidity from this
registry. Pressure, CO2, lux, and UV are already published so future rules can
use them without coupling fan code to a sensor driver.

Channel jobs are assigned exclusively above the device layer:

```text
Fan 1 current policy:
  automatic temperature + humidity demand

Fan 2 current policy:
  manual 0%, no enabled climate source

Fan 2 available circulation policy:
  interval/schedule demand, for example 30% every 10 minutes

Either channel later:
  temperature, humidity, CO2, schedule, or another registered demand source
```

Fan 2 can therefore use the same climate curves as Fan 1 by changing only its
automation configuration. The interval producer is already available for both
channels and remains disabled until a channel policy explicitly enables it.

## Current behavior

Fan 1 keeps its existing climate automation and manual override. Fan 2 is an
independent manual PWM channel. It starts at 0% and is not driven by temperature
or humidity. Rest Mode always forces both fans off.

Use HTTP or MQTT to set Fan 2:

```json
{"cmd":"set_fan_manual","fan":2,"percent":50}
```

Commands without `fan` keep the legacy Fan 1 behavior and do not change Fan 2.
Fan 2 tacho monitoring stays disabled until its tacho wire is connected.

## Automatic curve engine and persistent policies

Compiled defaults live in the fan modules; the effective policy for each fan is
stored independently in versioned NVS and can be changed through validated
local HTTP or MQTT commands. No server or product-dashboard editor has been
added. Fan 1 defaults to automatic control. Fan 2 defaults to manual 0%.

Each fan config contains its default mode, manual and base output, minimum and
maximum output, startup boost, ramp rates, minimum runtime, and stall limits.
Every fan has the same capacity of up to six curve rules. A rule contains:

- stable rule ID
- enabled flag
- registry signal
- direction (`ABOVE` or `BELOW`)
- target source (fixed value, active climate temperature, or active climate humidity)
- absolute lead before the target
- full-load distance beyond the target
- hysteresis
- `GENTLE`, `NORMAL`, or `AGGRESSIVE` curve style

The active day/night or local-grow climate targets remain the actual targets
for the current temperature and humidity rules, so existing grow presets keep
controlling the desired climate. Other registry signals can use a fixed target.
The firmware generates ten curve points for every enabled rule and interpolates
between them. All active rule demands are combined using `MAXIMUM`. Rest Mode,
sensor-safe behavior, and latched stall shutdown remain higher priority than
normal automatic demand.

Current Fan 1 defaults:

```text
mode: automatic
minimum / maximum: 25% / 100%
startup boost: 60% for 1.5 s
minimum runtime: 60 s
temperature: target lead 1.5 C, full load target + 7 C, hysteresis 0.5 C, normal
humidity: target lead 5%, full load target + 20%, hysteresis 3%, normal
interval: disabled (prepared default: 30% for 2 min every 10 min)
```

## Demand producers and interval operation

Automatic fan output is no longer produced by the curve engine alone. Each fan
evaluates independent producers and the highest percentage wins:

```text
curve rules ---+
interval -------+--> MAXIMUM --> arbiter
schedule -------+
base operation -+
```

The interval producer is based on ESP uptime and needs no network or valid wall
clock. Its policy defines enabled state, period, run duration, start
delay, and percentage. It stays inactive until the start delay has elapsed,
then requests the configured percentage during the run window of each period.
The arbiter's per-fan minimum runtime can extend a shorter interval run.

A channel with enabled sensor curve rules participates in sensor-fault safety.
A channel driven only by interval/base demand does not jump to sensor-safe
output because of an unrelated climate sensor fault. Rest Mode and device
faults still override every producer for every channel.

Each enabled sensor rule has its own maximum signal age and missing-signal
behavior. Invalid or stale values never enter curve calculation. `SAFE_OUTPUT`
requests the configured global sensor-safe output after a ten-second boot grace;
`IGNORE` disables only that demand. Daily schedule windows require valid system
time and otherwise request zero. Cross-midnight windows are supported.

The physical display, HTTP, and MQTT commands all update the same runtime fan
mode. Encoder previews are applied without an NVS write for every detent and
persisted when editing is completed.

## Stall protection

Stall protection is available independently for both fans when the respective
tacho input is enabled. When a fan is requested at 30% or more but remains
below 100 RPM for 8 seconds, the firmware latches a tacho fault and shuts that
fan down to 0%. The safety path sets PWM to 0 immediately instead of using the
normal slow operational ramp. The other fan continues operating normally.

The affected fan reports mode `FAULT`, reason `STALL OFF`, and
`tacho_fault: true` through HTTP and MQTT. Rest Mode does not silently clear the
fault. A deliberate manual or auto command for that fan acknowledges the fault
and permits one new start attempt; a reboot also clears it.

Fan 1 stall protection is active with the current PCB defaults. Fan 2 stall
protection becomes active only after its tacho wire is connected and
`FAN2_TACHO_ENABLED` is set to `1`.

## Generic per-fan automation

The curve evaluator now iterates a generic rule list instead of containing one
hard-coded temperature path and one hard-coded humidity path. The same engine
can evaluate temperature, humidity, CO2, pressure, lux, or UV registry signals
for either fan. `ABOVE` and `BELOW` rules are supported, as are fixed targets
and the active climate targets.

The current policies still contain only temperature and humidity rules. Fan 1
has both enabled; Fan 2 contains the same rule definitions with both disabled.
Changing policy data is therefore sufficient to give either channel the same
automatic job.

HTTP and MQTT status retain the compatibility fields
`temperature_demand_pct` and `humidity_demand_pct` and additionally expose the
winning generic rule ID. The configuration response includes the generic rule
list while retaining the original named temperature/humidity fields.

Example code-only rule shape:

```json
{
  "id": "CO2",
  "signal": "climate.co2_ppm",
  "enabled": true,
  "direction": "ABOVE",
  "target_source": "FIXED",
  "fixed_target": 900,
  "lead_before_target": 100,
  "full_load_beyond_target": 600,
  "hysteresis": 50,
  "curve": "NORMAL"
}
```

The server/dashboard UI is intentionally still outstanding. It can later edit
the existing policy API without changing the fan runtime architecture.
