# Fan Control Concept

## Layered implementation

The fan path is split into fixed layers while preserving the existing public
HTTP, MQTT, display, and control interfaces:

```text
fan_control.cpp
  evaluates temperature and humidity curves and produces demand percentages

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

Normal temperature, humidity, and base demands are already combined with
`MAXIMUM` by the control layer. The arbiter never reads sensors or writes PWM.
Its decision priority is exposed in Fan 1 and Fan 2 HTTP/MQTT status.

The hardware driver has no climate, grow phase, Rest Mode, MQTT, or HTTP
knowledge. The device block has no temperature or humidity knowledge. On a
confirmed RPM stall, the device block calls the driver's immediate emergency
stop and keeps the fault latched until a deliberate new fan command or reboot.

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

## Code-only automatic curve engine

The current implementation keeps all new fan settings in
`src/modules/fan_control.cpp`; no server or product-dashboard editor has been
added yet. Fan 1 defaults to automatic control. Fan 2 defaults to manual 0%.

Each fan config contains its default mode, manual and base output, minimum and
maximum output, startup boost, ramp rates, minimum runtime, and stall limits.
Temperature and humidity each have an independent source rule with:

- enabled flag
- absolute lead below the active climate target
- full-load distance above the active climate target
- hysteresis
- `GENTLE`, `NORMAL`, or `AGGRESSIVE` curve style

The active day/night or local-grow climate targets remain the actual targets,
so existing grow presets keep controlling the desired climate. The firmware
generates ten curve points for every enabled source rule and interpolates
between them. Temperature and humidity demands are combined using `MAXIMUM`.
Rest Mode, sensor-safe behavior, and latched stall shutdown remain higher
priority than normal automatic demand.

Current Fan 1 defaults:

```text
mode: automatic
minimum / maximum: 25% / 100%
startup boost: 60% for 1.5 s
minimum runtime: 60 s
temperature: target lead 1.5 C, full load target + 7 C, hysteresis 0.5 C, normal
humidity: target lead 5%, full load target + 20%, hysteresis 3%, normal
```

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

## Planned per-fan automation

The current temperature/humidity curve engine and per-fan arbiter provide the
first two automatic sources. Later versions can generalize these code-only
settings into configurable trigger rules. A rule would define:

- source, such as temperature, humidity, CO2, light, schedule or digital input
- comparison (`above`, `below`, `on` or `off`)
- threshold and hysteresis
- requested fan percentage
- optional activation delay and minimum runtime
- behavior when the source is missing or faulty

Multiple active rules should be combined by taking the highest requested fan
speed. Rest Mode and safety shutdowns always have higher priority.

Example future configuration:

```json
{
  "fan": 2,
  "mode": "automatic",
  "combine": "maximum",
  "triggers": [
    {
      "source": "temperature_c",
      "condition": "above",
      "threshold": 27.0,
      "hysteresis": 1.0,
      "output_percent": 60
    }
  ]
}
```

Recommended implementation order:

1. Persist independent mode and manual percentage for every fan.
2. Generalize the existing curve evaluator for additional registered sources.
3. Expose validated rule configuration through HTTP and MQTT.
4. Add dashboard controls for source, target, lead, full load, hysteresis and curve style.
5. Add automated tests for rule, arbiter, device, and fault behavior.
