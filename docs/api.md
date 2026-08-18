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
- optional RTC status when present
- grow mode
- fan state, winning demand producer/rule, interval demand, and schedule demand
- light state
- pump state and event-lock safety status
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
{"cmd":"set_fan_auto","fan":1}
```

```json
{"cmd":"set_fan_manual","fan":2,"percent":50}
```

If `fan` is omitted, Fan 1 is used for backward compatibility. Both fan modes
and manual values are persisted in the fan policy.

### Fan Policy

Both fans expose the same policy sections. Updates can be sent as a control
command over HTTP/MQTT, or without `cmd` to `POST /api/v1/config`.

```json
{"cmd":"set_fan_policy","fan":1,"section":"automation","base_percent":0,"minimum_run_ms":60000}
```

```json
{"cmd":"set_fan_policy","fan":1,"section":"rule","rule_index":0,"enabled":true,"signal":"climate.temperature_c","direction":"above","target_source":"climate_temperature","lead_before_target":1.5,"full_load_beyond_target":7.0,"hysteresis":0.5,"max_signal_age_ms":10000,"missing_signal":"safe_output","curve":"normal"}
```

```json
{"cmd":"set_fan_policy","fan":2,"section":"interval","enabled":true,"period_ms":600000,"run_ms":120000,"start_delay_ms":600000,"percent":30}
```

```json
{"cmd":"set_fan_policy","fan":2,"section":"schedule","schedule_index":0,"enabled":true,"start_minute":480,"end_minute":1200,"percent":30}
```

Schedules use local minutes after midnight (`0..1439`), require valid system
time, and may cross midnight. Each fan has three schedule slots and six curve
rule slots. Base, curve, interval, and schedule requests are combined by taking
the highest requested percentage.

Device behavior can also be configured per fan:

```json
{"cmd":"set_fan_policy","fan":1,"section":"device","minimum_percent":25,"maximum_percent":100,"startup_boost_percent":60,"startup_boost_ms":1500,"ramp_up_pwm_step":4,"ramp_down_pwm_step":2,"ramp_interval_ms":100,"stall_minimum_check_percent":30,"stall_minimum_rpm":100,"stall_fault_delay_ms":8000}
```

Reset one fan to the compiled safe defaults:

```json
{"cmd":"set_fan_policy","fan":2,"section":"reset"}
```

`GET /api/v1/config` returns the complete effective policy, policy schema,
storage readiness, device settings, rules, interval, and schedules. Updates
are normalized to safe limits, stored as one versioned NVS policy per fan, and
applied after safely stopping that channel.

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

Pump tests do not count as scheduled pump events. They are blocked by Rest Mode and still obey the hard runtime limit.

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

### Warning Limits And Deprecated Legacy Fan Curve

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

The warning-limit fields remain supported. The five legacy `fan_pct_*` curve
fields are retained for compatibility but are not consumed by the layered fan
controller. New integrations must use the fan-policy rule API above.

### RTC Config

```json
{"cmd":"set_rtc_config","enabled":true}
```

```json
{"cmd":"set_rtc_config","enabled":false}
```

RTC support is optional and disabled by default. It is intended for DS3231/DS1307-compatible modules at I2C address `0x68`. When enabled and valid, the RTC can seed ESP system time at boot while NTP remains the primary online time source. Status and MQTT telemetry include `time_source` and an `rtc` object with `enabled`, `present`, `valid`, `used_for_boot`, `last_read_ok`, and `last_write_ok`.

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
