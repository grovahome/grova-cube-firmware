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

## General board UI (2026-11-12)

The ESP web app has three views: overview, devices/sensors and settings. Overview
shows named device cards (including the two existing dedicated PWM fan controls)
and current measurements with readable labels. Controls expand per device;
timed durations are entered in seconds and converted to integer milliseconds.
Assignment is edited separately, with unoccupied slots collapsed and only fields
relevant to the selected profile visible. Supported I2C sensors are listed with
readiness and their actual mux channel number (0..7; system bus separate).

WLAN, clock and display are settings. MQTT, existing fan rules/targets and raw
output diagnostics are under Advanced. No fixed grow phases or application-wide
modes are presented on the overview; general automation is still deferred.

The UI calls the existing Rest interlock "Ausgänge gesperrt". Internal Rest
storage and `set_rest_mode` API stay compatible. Every lock/release request also
clears both saved fan demands to manual zero; releasing must not implicitly
resume a previous fan setpoint or rule. Existing fan rule configuration remains
stored and can be explicitly selected under Advanced after release. If saving
the cleared demands fails, release is rejected and the board stays locked.

`stop_all` cancels assigned-device requests, switches off all generic output
channels (including free commissioning channels), saves both fans as manual zero
and forces the fan drivers off. It preserves the existing lock state on success.
A failed fan-mode save falls back to the lock. It is not an independent electrical
emergency stop and does not certify a physical fan stop at zero PWM.

When the browser cannot refresh status, activation, assignment save, release and
global stop are disabled and the output state is marked unknown. Previously
received values may remain visible; the UI does not claim that a communication
failure stopped the physical loads.

Validation: production Founder API/driver tests cover global stop, saved fan
zero demands, release without restarting old requests and fail-closed storage
errors. Desktop/mobile browser simulation covers navigation, assignment payloads,
profile-specific hidden fields, draft retention, timed start in seconds, global
stop, assigned output lockout and loss-of-connection controls. No hardware was
flashed or physically tested by these checks.

## Fixed output occupation (2026-11-12)

The latest editor replaces arbitrary device-slot selection with the real board
connectors, in this order: 12 V output 1, 12 V output 2, 5 V output, the two
0-10 V outputs, and both dedicated PWM/tacho fan connectors. Each connector has
an occupation checkbox. Only occupied connectors show device properties.
Electrical voltage and connection type cannot be edited or reassigned: the
client derives `voltage` / `power_voltage` from the board capabilities, and the
firmware still independently rejects incorrect voltage, type or duplicate claims.
Compatible device profiles are filtered by that fixed connector's capability.

Occupation is configuration, not an on command. Edits require the output lock.
Unchecking a connector hides its properties in the draft. After **Belegung
übernehmen** saves successfully, its device disappears from the overview and
its assignment is removed persistently. A failed save preserves the previous
active configuration; merely toggling or navigating never applies an unsaved
draft. Unchanged device IDs are preserved internally when saving, while newly
occupied connectors receive free internal IDs; these slots are no longer user
choices. Runtime limits are entered in seconds and transmitted as milliseconds.

Optional switched power for a 0-10 V light still consumes a real 5/12 V connector.
That connector is visibly linked to the controlling 0-10 V device and cannot be
independently occupied; its voltage also comes from board metadata. Removing
the control-device assignment releases its paired power connector.

PWM fan occupation and names are persisted in the same atomic configuration
record as the other assignments (`fan_ports`: two enabled/name objects). The
fan control implementation remains the existing PWM/tacho service. Disabled
fan ports are hidden on the overview and reject positive manual, auto and
policy commands. Zero/stop requests remain possible. An older configuration
without `fan_ports` restores both as unoccupied; the user must explicitly occupy
connected fans. A config command omitting `fan_ports` preserves the currently
loaded fan-port assignments. No additional hardware interface is introduced.

Validation: production host tests cover fixed voltage/type enforcement, native
fan activation/deactivation, disabled-port command rejection, durable reload and
failed-save rollback. Browser simulation covers all seven fixed connectors,
absence of voltage selectors, correct 5/12 V payloads, optional power reservation,
removal only after successful save, new assignments, named PWM fans, disabled
editing while unlocked and mobile layout. Physical acceptance stays open.
