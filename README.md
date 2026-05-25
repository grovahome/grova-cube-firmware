# GROVA Cube Firmware

Standalone PlatformIO firmware for the GROVA Cube prototype.

The cube is designed to work locally on its own first: sensor reading, OLED/encoder UI, grow modes, light/fan/pump automation, pump safety, HTTP fallback controls, OTA updates, and optional MQTT telemetry/commands.

## Current Baseline

- Board: ESP32 DevKit (`esp32dev`)
- Framework: Arduino
- Current firmware version: `v1.0.1`
- First verified standalone baseline: `v1.0.0`
- Local fallback UI/API: enabled on the ESP web server
- MQTT: optional interface for a later local server, app, or cloud bridge

## Features

- DHT22 temperature and humidity sensing
- 0.96 inch OLED status display
- Rotary encoder local UI
- Grow modes: germination, growth, harvest
- Automatic and manual light control
- Automatic and manual fan control
- Pump schedule, pump test, and manual stop
- Pump safety limits for automatic watering
- Persistent climate targets, warning limits, schedules, and fan curve
- Local ESP web UI and versioned HTTP APIs
- OTA firmware updates
- Optional MQTT telemetry, commands, and acknowledgements

## Setup

Copy the example secrets file and fill in local credentials:

```bash
cp include/secrets.example.h include/secrets.h
```

`include/secrets.h` is ignored by Git and must not be committed.

## Hardware

Current prototype hardware known to the firmware:

| Part | Current component / role | Firmware pins / notes |
| --- | --- | --- |
| Main controller | ESP32 DevKit-compatible board | PlatformIO board: `esp32dev` |
| Temperature/humidity sensor | DHT22 | Data on GPIO 4 |
| Display | 0.96 inch I2C OLED, SSD1306-compatible, 2.2-5.5 V | SDA GPIO 19, SCL GPIO 18 |
| Local input | Rotary encoder with push button | CLK GPIO 32, DT GPIO 33, SW GPIO 16 |
| Light output | 12 V full-spectrum LED grow light panel, switched through MOSFET module | GPIO 26 |
| Pump output | 5 V micro peristaltic pump, switched through MOSFET module | GPIO 27 |
| MOSFET switching | 15 A / 400 W MOSFET module | Used for light and pump outputs |
| Fan output | Noctua NF-A8 PWM, chosen for low noise | PWM GPIO 25 |
| Fan tachometer | Optional fan tacho signal | GPIO 35, currently disabled in firmware |
| Power supply | Generic 12 V wall power supply | Exact current rating to confirm |
| Protection parts | No flyback diode, fuse, level shifting, or extra protection parts currently installed | Important to revisit before production use |

## Pinout

| GPIO | Function |
| --- | --- |
| GPIO 4 | DHT22 data |
| GPIO 16 | Encoder switch |
| GPIO 18 | OLED SCL |
| GPIO 19 | OLED SDA |
| GPIO 25 | Fan PWM |
| GPIO 26 | Light MOSFET |
| GPIO 27 | Pump MOSFET |
| GPIO 32 | Encoder CLK |
| GPIO 33 | Encoder DT |
| GPIO 35 | Fan tachometer input, currently disabled |

Current fixed firmware defaults:

```text
Light on hour:       08:00
Light off hour:      20:00
Pump run time:       08:45
Pump runtime:        10 s
Pump test runtime:   5 s
Pump max runtime:    10 s
Pump max auto runs:  2 per day
Pump min interval:   6 h
Pump startup lock:   10 min
Display sleep:       60 s
Fan idle:            25 %
Fan min active:      35 %
Fan max:             100 %
Sensor fail fan:     60 %
```

Hardware details still to confirm:

- exact ESP32 development board model
- OLED I2C address
- exact MOSFET module model/type
- pump current draw and how 5 V is supplied from the 12 V system
- grow light power/current rating
- main power supply current rating

## Safety Notes

This is prototype firmware for a local grow cube, not production hardware.

- The current prototype has no flyback diode, fuse, level shifting, or extra protection parts installed.
- Verify MOSFET, pump, grow light, wire, connector, and power supply current ratings before unattended use.
- A 5 V pump in a 12 V system needs a suitable 5 V supply path.
- Revisit fan tachometer wiring and signal quality before enabling tacho-based behavior.
- Add electrical protection and safer power distribution before any production or long-term unattended setup.

## Configuration

Local secrets live in:

```text
include/secrets.h
```

Create it from:

```text
include/secrets.example.h
```

MQTT is optional. The cube can keep running locally through its display, encoder UI, schedules, and ESP web fallback even if MQTT is not connected.

Persistent runtime settings are stored on the ESP32 through Preferences/NVS:

- grow mode
- light schedule and mode
- pump schedule and duration
- climate day/night targets
- warning limits
- fan curve

## Build

```bash
pio run -e esp32dev
```

## USB Upload

```bash
pio run -e esp32dev -t upload
```

## OTA Upload

The OTA environment currently targets the local prototype cube. Adjust `upload_port` in `platformio.ini` if needed.

```bash
pio run -e esp32dev_ota -t upload
```

## Local APIs

The cube exposes versioned local HTTP APIs when connected to Wi-Fi:

```text
GET  /api/v1/status
POST /api/v1/control
GET  /api/v1/config
POST /api/v1/config
```

Example control commands:

```json
{"cmd":"set_fan_manual","percent":50}
```

```json
{"cmd":"set_fan_auto"}
```

```json
{"cmd":"pump_test","action":"start"}
```

```json
{"cmd":"set_light_schedule","on_hour":8,"off_hour":20}
```

MQTT is optional and uses the topic shape:

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
```

## Roadmap

- Move from DHT22 to a BME sensor later for more robust climate sensing.
- Add temperature and humidity calibration.
- Store and expose daily min/max values directly on the cube.
- Improve fan tachometer reliability before enabling tacho-based features.
- Support easier runtime configuration for MQTT host, port, cube ID, and credentials.
- Prepare TLS/certificates and provisioning for future app or cloud operation.
- Add electrical protection and better power distribution before production use.

## Documentation

- [API reference](docs/api.md)
- [Display and encoder UI](docs/display-ui.md)
- [Pump safety](docs/pump-safety.md)
- [Changelog](CHANGELOG.md)

## Repository Scope

This repository intentionally contains only firmware-relevant files.

Included:

- `src/`
- `include/`
- `lib/`
- `test/`
- `platformio.ini`
- `include/secrets.example.h`

Excluded:

- real secrets in `include/secrets.h`
- server, dashboard, Grafana, InfluxDB, Node-RED, and deployment files
- local databases, logs, backups, and `.env` files
