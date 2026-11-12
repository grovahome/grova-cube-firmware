# Founder device assignment

Status: implemented for manual operation on the existing Founder PCB interfaces.
Physical board acceptance remains OPEN. No automation engine is introduced.

## Separation of responsibilities

1. Board profile: pins and electrical output capabilities, without application roles.
2. Device catalog: supported connection types, commands and protection limits.
3. Assignment: a named device owns one compatible output (optionally two for light).
4. Device implementation: converts manual device commands into protected output requests.
5. Future automation: will request device actions through the same implementations.

The switched 5 V and 12 V channels can support the same device functions. Their
supply voltage must match the actual load; assignment never converts voltage.
For example, a 5 V DC pump uses `OUT5_1`, while a 12 V DC pump uses `OUT12_1`
or `OUT12_2`. Both use profile `pump` and the same timed pump implementation.
These are low-side switched DC connections, not mains relays or arbitrary power
supplies. Verify connector polarity, common ground, inrush, flyback protection,
per-output and total current limits before connecting a real load. Current
ratings are deliberately reported as unknown until the hardware limits are verified.

0-10 V outputs provide a control signal, not device power. External equipment
must have a compatible 0-10 V input and suitable external supply. A 0 V request
is not proof that its motor or light physically stops; verify the equipment's
minimum-speed / dim-to-off behavior. No measured output-voltage or load-current
feedback is available in this implementation.

## Implemented catalog

| Profile | Equipment | Compatible connection | Behavior / additional protection |
| --- | --- | --- | --- |
| `pump` | DC pump | Switched 5 V or 12 V | Timed pulse only, boot lock 10 min, maximum 10 s (configurable lower), persistent recent event IDs |
| `valve_pulse` | Monostable, normally closed DC valve | Switched 5 V or 12 V | Timed pulse only, maximum 10 s (configurable lower), persistent recent event IDs |
| `light_switch` | DC light | Switched 5 V or 12 V | Boolean on/off |
| `light_0_10v` | Light with compatible dimming input | 0-10 V | 0-100%; optional exclusively assigned 5/12 V power output; power on before setpoint, setpoint zero before power off |
| `fan_switch` | DC fan | Switched 5 V or 12 V | Boolean on/off; no tacho feedback |
| `fan_0_10v` | Fan with compatible control input | 0-10 V | 0-100%; no tacho feedback |
| `switch` | General DC switching load | Switched 5 V or 12 V | Boolean on/off; no pump, thermal or load-specific protections |

This is a finite catalog of implemented behaviors, not a claim that every
possible electrical load is supported. Do not select a general switch to bypass
pump/valve protections. Heaters, humidifiers, latching valves, reversing motors,
and 0-10 V pumps are not selectable native profiles yet: their required feedback,
failure handling and protections must be defined and implemented first.
The two dedicated PWM/tacho fan connectors retain the existing fan service and
its controls. They are not interchangeable with the switched or 0-10 V outputs.

## Assignment and manual controls

The local web app has five device slots with name, profile, output and load
voltage. Timed profiles can lower the runtime limit. Compatible outputs are
filtered in the form; the firmware independently validates every request.
Assignment is saved only in Rest Mode and only via the local control API.
The complete mapping is validated for duplicate slots, duplicate output claims,
output type, voltage and profile limits, then saved as one versioned NVS record.
Only a successful save replaces the active mapping. All outputs are then off.
A reboot restores the mapping but never restores an active device request.
Invalid stored mappings leave every channel unassigned and report a settings error.

Device commands are `set_device`, `run_device`, `stop_device`. Timed starts require
`runtime_ms` and a unique `event_id`; they cannot be prolonged by sending a
normal on/set command or another start while running. The last 16 accepted
slot/event pairs are reserved in NVS before energizing, including across reboot.
This bounded history is not unlimited exactly-once delivery. Remote commands
still require unique command identity and synchronized, unexpired deadlines;
remote timed starts additionally require deadlines within 60 seconds.

All profiles inherit output ownership and Rest/setup/OTA interlocks. A shared
1 ms hardware timer (group 1 / timer 0, reserved in the Founder target) clears
switched GPIOs at timed deadlines without relying on the application, Wi-Fi,
HTTP or MQTT loop. Failed timer/PWM initialization blocks output activation.
This is firmware protection, not an independent electrical watchdog or proof
against complete MCU failure. Actual cutoff latency and load behavior need
measurement on the real board. Safety/water input semantics are not implemented;
there is no claim of dry-run, flow, reservoir or overtemperature detection.

Assigned channels cannot be driven or stopped through raw output commands.
`test_output` is a local-only commissioning command for unassigned channels;
the old generic `set_output` command is rejected. The UI marks these as free
output tests. Test them without protection-dependent loads. The device catalog,
capabilities, assignments, applied state, remaining pulse time and blocking
reason are available in the normal status response.

## Validation

Production C++ tests cover voltage/type mismatches, duplicate claims, mapping
write rollback and reload, active-state remapping rejection, raw bypass denial,
pump boot lock and runtime limit, event write failure, replay rejection after
reload, timer cutoff without an application loop, explicit stop, composite
light power sequencing outcomes, Rest shutdown and corrupt stored mapping.
A separate process tests timer initialization failure. Browser simulation uses
the production status JSON and checks device selection, payloads, draft retention,
assigned-channel lockout, pulse start/stop and mobile layout.

Pending hardware checks: 5/12 V load compatibility and current limits, physical
pulse cutoff, every channel's off state through reset/setup/OTA, 0-10 V range
and zero behavior, optional light power switching, and actual device-specific
failure behavior. These tests do not close the separate first-board checklist.
