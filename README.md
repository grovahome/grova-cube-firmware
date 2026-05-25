# GROVA Cube Firmware

Standalone PlatformIO firmware for the GROVA Cube prototype.

The cube is designed to work locally on its own first: sensor reading, OLED/encoder UI, grow modes, light/fan/pump automation, pump safety, HTTP fallback controls, OTA updates, and optional MQTT telemetry/commands.

## Current Baseline

- Board: ESP32 DevKit (`esp32dev`)
- Framework: Arduino
- Stable firmware baseline: `v1.0.0`
- Local fallback UI/API: enabled on the ESP web server
- MQTT: optional interface for a later local server, app, or cloud bridge

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

MQTT is optional and uses the topic shape:

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
```

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
