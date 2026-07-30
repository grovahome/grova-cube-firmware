# GROVA CORE Technical Documentation

This document collects the technical firmware details that are intentionally kept out of the front-page README.

For setup and flashing, see [setup.md](setup.md). For lower-level endpoint details, see [api.md](api.md).

## Current Baseline

| Component | Value |
| --- | --- |
| Firmware release | `v1.1.0` |
| Firmware branch | `grova-core-v1` |
| Framework | Arduino |
| Build system | PlatformIO |
| Operation model | Local-first |
| Persistent storage | ESP32 Preferences/NVS |
| MQTT library | PubSubClient 2.8 |

## Supported Sensors

Current firmware supports:

- AHT20 temperature and humidity
- Optional SHT41/SHT4x temperature and humidity, disabled by default
- DHT22 temperature and humidity
- BME280 pressure, temperature and humidity
- BMP280 pressure and temperature
- Optional DS3231/DS1307-compatible RTC at `0x68`

The sensor module reads the enabled primary temperature/humidity source in this order:

1. SHT41/SHT4x, when enabled and detected
2. AHT20, when enabled and detected
3. DHT, when enabled
4. BME280 humidity/temperature fallback, when Bosch support is enabled and no primary source exists

For the current PCB cube, AHT20 remains the default primary temperature/humidity sensor. SHT41 support is compiled only when `GROVA_SENSOR_SHT41=1` is set locally. The Bosch sensor is used mainly for pressure.

## Local HTTP API

The cube exposes versioned local HTTP APIs when connected to Wi-Fi:

```text
GET  /api/v1/status
POST /api/v1/control
GET  /api/v1/config
POST /api/v1/config
GET  /api/v1/local-presets
```

Compatibility aliases under `/api/...` remain available where implemented.

Example control commands:

```json
{"cmd":"set_fan_manual","percent":50}
```

```json
{"cmd":"set_fan_manual","fan":2,"percent":70}
```

```json
{"cmd":"set_output","output":"aux_12v","state":true}
```

```json
{"cmd":"set_output","output":"aux_5v","state":false}
```

```json
{"cmd":"set_rest_mode","enabled":true}
```

If `fan` is omitted, fan commands apply to all enabled fan channels for backward compatibility.

See [api.md](api.md) for endpoint and payload details.

## MQTT Topics

Topic structure:

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
grova/v1/cubes/{cube_id}/availability
homeassistant/device/{cube_id}/config
```

All telemetry, commands, acknowledgements, availability messages and discovery payloads are scoped by `cube_id`.

MQTT remains optional. Critical automation logic runs locally on the controller.

## Home Assistant Discovery

Home Assistant MQTT Discovery is available when MQTT discovery is enabled.

The cube publishes one retained device discovery payload and groups entities under one Home Assistant device.

Discovery topic:

```text
homeassistant/device/{cube_id}/config
```

Availability topic:

```text
grova/v1/cubes/{cube_id}/availability
```

Availability payloads:

```text
online
offline
```

The MQTT Last Will is set to `offline`. On connect, the cube publishes `online`, republishes discovery and sends telemetry.

## Native Rest Mode

Rest Mode is a persistent firmware state for parking a cube between grows without overwriting the stored grow mode or manual settings.

Runtime control through HTTP or MQTT:

```json
{"cmd":"set_rest_mode","enabled":true}
```

```json
{"cmd":"set_rest_mode","enabled":false}
```

When enabled:

- light output is forced off
- fan PWM outputs are forced to 0 percent
- automatic pump runs and pump tests are blocked
- any running pump is stopped
- temperature and humidity warning limits are suspended
- the current grow mode and manual settings remain stored

Status and MQTT telemetry expose `rest_mode.enabled`, `rest_mode.mode` and `rest_mode.reason`.

## Local Offline Preset Mode

The firmware can store compact grow presets directly on the ESP32 and execute one active grow locally.

Limits:

| Item | Limit |
| --- | --- |
| Local ESP preset slots | 5 |
| Maximum phases per preset | 10 |
| Maximum pump events per phase | 5 |
| Active local run | 1 |

Runtime control through HTTP or MQTT:

```json
{"cmd":"set_local_preset","slot":0,"payload_hex":"..."}
```

```json
{"cmd":"clear_local_preset","slot":0}
```

```json
{"cmd":"start_local_run","slot":0,"start_at_ms":1780000000000,"run_id":"run-...","revision":123456}
```

```json
{"cmd":"stop_local_run"}
```

```json
{"cmd":"pause_local_run"}
```

```json
{"cmd":"resume_local_run"}
```

During a local run, the ESP applies phase grow mode, climate targets, light schedule and due pump events itself. Server-side stop, pause/resume commands and active-preset resyncs can still intervene while the cube is online.

## Pump Safety

Automatic pump runs use an event lock instead of a hard daily or interval throttle.

Important safety behavior:

- each scheduled pump event can start once per date/minute
- distinct scheduled events are not blocked by a daily count or fixed interval
- automatic pump runs are blocked during startup lock
- automatic pump runs are blocked without time sync
- automatic pump runs are blocked in Harvest mode
- automatic pump runs are blocked in Rest Mode
- automatic pump duration is clamped to the hard maximum runtime

See [pump-safety.md](pump-safety.md) for deeper details.

## Optional RTC Support

The firmware supports DS3231/DS1307-compatible RTC modules on the shared I2C bus at address `0x68`.

RTC support is disabled by default so cubes without RTC hardware behave like NTP-only devices. Enable it only when an RTC module is fitted.

Runtime control through HTTP or MQTT:

```json
{"cmd":"set_rtc_config","enabled":true}
```

```json
{"cmd":"set_rtc_config","enabled":false}
```

When enabled and valid, the RTC can seed ESP system time during boot. NTP remains the primary online time source and refreshes the RTC later.

Telemetry exposes `time_source` plus `rtc.enabled`, `rtc.present`, `rtc.valid`, `rtc.used_for_boot`, `rtc.last_read_ok` and `rtc.last_write_ok`.

## Hardware Profiles

The built-in defaults are selected with `GROVA_BOARD_PCB_V1`.

Use `GROVA_BOARD_PCB_V1=1` for the GROVA PCB v1 defaults and `GROVA_BOARD_PCB_V1=0` for the legacy DHT wiring defaults.

Older local `board_config.h` files that define `GROVA_BOARD_PCB_V2` are accepted as a backwards-compatible alias, but new configs should use `GROVA_BOARD_PCB_V1`.

See [firmware-profiles.md](firmware-profiles.md) and [hardware/README.md](hardware/README.md) for the current hardware reference.

## Repository Scope

Included in this firmware repository:

```text
src/
include/
lib/
test/
docs/
platformio.ini
README.md
CHANGELOG.md
LICENSE
```

Excluded from this firmware repository:

```text
real credentials
local board overrides
server deployment files
dashboard backend
databases
logs
backups
```
