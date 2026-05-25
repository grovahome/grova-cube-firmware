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
