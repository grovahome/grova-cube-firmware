# Fan Control Concept

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

Each fan should later have a mode (`manual` or `automatic`) and a list of
configurable trigger rules. A rule defines:

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
2. Add a generic trigger evaluator outside the GPIO/PWM driver.
3. Expose validated rules through HTTP and MQTT.
4. Add dashboard controls for source, threshold, hysteresis and speed.
5. Add sensor-failure policies, reason telemetry and automated tests.
