# GROVA CORE Technical Documentation

This document collects the technical firmware details that are intentionally kept out of the front-page README.

For setup and flashing, see [setup.md](setup.md). For lower-level endpoint details, see [api.md](api.md).
For I2C module autodiscovery, see [i2c-discovery.md](i2c-discovery.md).

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

- SHT41/SHT4x temperature and humidity
- AHT20/AHTx0 temperature and humidity
- SCD41 CO2, temperature and humidity
- BME280 pressure, temperature and humidity
- BMP280 pressure and temperature
- VEML7700 ambient light / lux
- LTR390 UV index and UV/ALS raw sensing
- Optional DS3231/DS1307-compatible RTC at `0x68`

I2C discovery reports known GROVA module addresses for SHT41/SHT4x, AHT20/AHTx0,
VEML7700, SCD41, BME/BMP, LTR390, OLED, and RTC. No I2C sensor is required for the
firmware to boot. Stable telemetry channels such as CO2, lux, UV index, and
pressure use `null` while no matching sensor is connected or no valid reading
has been received yet.

The sensor module reads the enabled primary temperature/humidity source in this order:

1. SHT41/SHT4x, when compiled in and detected
2. AHT20/AHTx0, when compiled in and detected
3. SCD41, when detected and a measurement is available
4. BME280 humidity/temperature fallback, when Bosch support is enabled and present

For PCB/Founder builds, the planned I2C sensor families are compiled in by
default and detected at runtime. The user should only need to connect a known
module and restart the cube or wait for a rescan.

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

The command above controls Fan 1 for backward compatibility. Explicit fan
selection is recommended:

```json
{"cmd":"set_fan_auto","fan":1}
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

If `fan` is omitted, the command applies to Fan 1. Fan 2 currently defaults to
manual 0% and is only changed by an explicit `"fan":2` command.

## Fan Control

The current PCB defaults use Fan 1 as the connected automatic fan. Fan 2 is an
available independent PWM channel but defaults to manual 0% until a second fan
is fitted.

Fan 1 automatic demand is calculated independently from temperature and
humidity. Both rules use the active day/night or local-program climate targets,
automatically generated ten-point curves, hysteresis, startup boost, and a
minimum runtime. The higher temperature or humidity demand wins.

The final demand is resolved locally using this priority order:

```text
DEVICE_FAULT -> REST -> SENSOR_SAFETY -> MANUAL -> AUTOMATIC -> IDLE
```

Fan status under HTTP and MQTT includes current/target percentage, RPM, reason,
decision priority, temperature demand, humidity demand, and tacho fault state.

When an enabled tacho remains below the configured minimum RPM while the fan
should be running, the device layer latches a stall fault and the hardware
driver immediately sets PWM to 0. A deliberate new manual/auto command for that
fan acknowledges the fault and permits a new start attempt. Fan 2 stall
monitoring remains disabled until its tacho wire is physically connected and
`FAN2_TACHO_ENABLED` is enabled.

The current curve and device settings are code-only in
`src/modules/fan_control.cpp`; the server/dashboard does not edit them yet. See
[fan-control-concept.md](fan-control-concept.md) for the layered architecture
and current defaults.

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

The active built-in defaults target GROVA PCB v1 / Founder Edition. Legacy DHT
hardware is frozen at commit `11eacaf` and should not receive updates from the
current source line.

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
