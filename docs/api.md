# GROVA Cube API

The cube is designed to work locally first. HTTP APIs are available on the ESP web server when Wi-Fi is connected. MQTT is optional and can be used by a local server, app, automation layer, or future cloud bridge.

## HTTP Endpoints

```text
GET  /api/v1/status
POST /api/v1/control
GET  /api/v1/config
POST /api/v1/config
```

Legacy aliases without `/v1` may exist on the prototype firmware, but new integrations should use `/api/v1/...`.

## Status

```text
GET /api/v1/status
```

Returns the current cube state as JSON, including:

- firmware version and build metadata
- temperature and humidity
- active climate targets
- warning and health state
- Wi-Fi/time status
- grow mode
- fan state
- light state
- pump state and safety counters
- sensor state

## Control

```text
POST /api/v1/control
```

Send a JSON command body.

### Grow Mode

```json
{"cmd":"set_grow_mode","mode":"GROWTH"}
```

Supported modes:

```text
GERM
GERMINATION
GROWTH
GROW
HARVEST
```

### Light Mode

```json
{"cmd":"set_light_mode","mode":"AUTO"}
```

Supported modes:

```text
AUTO
ON
OFF
MAN_ON
MAN_OFF
```

### Fan

```json
{"cmd":"set_fan_auto"}
```

```json
{"cmd":"set_fan_manual","percent":50}
```

`percent` is clamped to the firmware-supported fan range.

### Light Schedule

```json
{"cmd":"set_light_schedule","on_hour":8,"off_hour":20}
```

Hours are whole-hour values from `0` to `23`.

### Pump Schedule

```json
{"cmd":"set_pump_schedule","hour":8,"minute":45}
```

Minutes should use 5-minute steps.

### Pump Duration

```json
{"cmd":"set_pump_duration","duration_s":10}
```

Automatic pump duration is limited to `1..10` seconds.

### Pump Test

```json
{"cmd":"pump_test","action":"start"}
```

```json
{"cmd":"pump_test","action":"stop"}
```

Pump tests do not count toward the automatic daily limit or interval, but still obey the hard runtime limit.

### Climate Targets

```json
{
  "cmd": "set_climate_targets",
  "day_temp_c": 23.0,
  "night_temp_c": 20.0,
  "day_hum_pct": 60,
  "night_hum_pct": 55
}
```

Climate targets are the fan automation setpoints.

### Warning Limits And Fan Curve

```json
{
  "cmd": "set_config",
  "temp_min_c": 18.0,
  "temp_max_c": 28.0,
  "hum_min_pct": 45,
  "hum_max_pct": 85,
  "temp_over_c_0": 0.0,
  "hum_over_pct_0": 0,
  "fan_pct_0": 25
}
```

The firmware supports five fan curve points using suffixes `_0` through `_4`.

## MQTT

Topic structure:

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
```

MQTT command payloads use the same command shape as the HTTP control API. Include a `cmd_id` if the caller wants to match an ACK:

```json
{"cmd_id":"example-1","cmd":"set_fan_manual","percent":50}
```

ACK topic:

```text
grova/v1/cubes/{cube_id}/ack
```

Example ACK:

```json
{
  "cube_id": "grova-cube-001",
  "cmd_id": "example-1",
  "ok": true,
  "response": {
    "ok": true,
    "message": "fan manual updated"
  }
}
```
