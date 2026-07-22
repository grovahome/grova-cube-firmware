# GROVA Cube Firmware

Standalone PlatformIO firmware for GROVA Cube ESP32 controllers.

The firmware is local-first: each cube can run its sensors, OLED/encoder UI,
grow modes, light/fan/pump automation, pump safety, HTTP fallback controls, and
OTA updates on its own. MQTT telemetry and commands are optional and are used by
the GROVA dashboard/server when enabled.

---

## 🎯 System Functionality & Core Purpose

This firmware operates as a local-first automation system designed to monitor, regulate, and control small-scale indoor agricultural environments, such as microgreen trays and modular cultivation cabinets. The software operates entirely offline, managing real-time data collection from localized sensor arrays and handling automated power distribution to low-voltage peripherals.

### Automated Climate & Irrigation Controls:
* **Precision Interval Irrigation:** Manages low-voltage peristaltic dosing pumps through a configurable time-interval and duration-taktung matrix. It handles precise watering schedules (e.g., executing a 15-second pump cycle every 6 hours) to saturate root mediums uniformly via capillary action without causing water accumulation.
* **Integrated Pump Safety Engine:** Features independent software watchdogs that actively monitor the execution times of irrigation cycles. If a pump continues running beyond a strict predefined threshold, the system triggers an emergency override and cuts power to the line, protecting the crop from flooding and the pump motor from running dry.
* **Dual-Zone Climate & Ventilation:** Controls 2x separate N-Channel MOSFET power lines for 12V or 5V DC heating elements and high-power LED arrays. Additionally, it drives 2x independent, hardware-defined PWM fan outputs to maintain continuous air turnover, ensuring uniform temperature distribution across the growth area.
* **State-Driven Grow Recipes:** Automates predefined growth schedules. The firmware transitions variables automatically based on the active state machine, shifting timers and climate parameters from the initial Dark/Blackout phase into active vegetative light schedules.
* **Localized Sensor Integration:** Collects and processes environmental telemetry (including temperature, relative humidity, and barometric pressure from Bosch BME280 or compatible arrays) to drive local feedback loops for ventilation and heating adjustments.

### Decentralized Multi-Device Architecture:
* **Complete Offline Autarky:** Every controller operates as a self-sustaining master node. All automated cycles and hardware timers are tracked locally via an onboard DS3231 RTC (Real-Time Clock), ensuring uninterrupted operations and precise timekeeping even during a complete breakdown of the local Wi-Fi router or the central server.
* **Parallel Swarm Telemetry:** The firmware separates system states, commands, acknowledgments (ACKs), and historical datasets via distinct hardware identifiers (`MQTT_CUBE_ID`). This structural separation allows a single centralized dashboard or local server instance to monitor, manage, and visualize multiple physical boxes simultaneously in real-time.

## 🔌 Hardware Compatibility & Alpha DevKit
While this unified firmware is abstracted to run on custom DIY breadboard setups via settings, it is fully optimized for the proprietary **GROVA Core Baseboard** (a compact 10x10cm application shield for standard 38-pin ESP32 NodeMCUs).

**Key hardware features supported by this firmware:**
* **Isolated Multi-Rail Power:** 4x software-defined N-Channel MOSFET outputs for silent, relay-free switching (2x 12V for high-efficiency LED dimming/heavy loads, 2x 5V for inductive peristaltic pumps/solenoid valves).
* **Dedicated Fan Hubs:** 2x independent hardware PWM fan outputs for precise speed control, fully separated from the main MOSFET rails.
* **Cascaded I2C Bus:** 4x independent, logic-buffered I2C channels with full bidirectional 3.3V/5V level-shifting for safe sensor and display integration.
* **Offline Timekeeper:** Onboard DS3231 RTC (Real-Time Clock) port with hardware-decoupled bypass capacitors to secure automated schedules during total Wi-Fi or local router drops.

👉 **[⚡ JOIN THE WAITLIST FOR BATCH #01 (AUGUST DROP) ON GROVAHOME.COM](https://grovahome.com)**
*A small alpha test run of 50 fully assembled plug-and-play developer kits (including the matching 38-pin ESP32 microcontroller) will be released end of August if there is enough interest. Sign up on our landing page to secure priority notification.*

## Current Baseline

- Board target: ESP32 DevKit compatible (`esp32dev`)
- Framework: Arduino
- Firmware version: `v1.0.1`
- Active branch: `grova-core-v1`
- Local fallback UI/API: enabled on the ESP web server
- MQTT: enabled per production cube profile, disabled in local test profile
- Multi-cube support: cube state, topics, ACKs, history, and commands are
  separated by `MQTT_CUBE_ID`

## Active Cube Profiles Examples

| Cube ID | Hardware | Sensors | Fan | OTA target | OTA IP |
| --- | --- | --- | --- | --- | --- |
| `grova-cube-001` | Legacy cube | DHT22 temperature/humidity | 1 fan | `grova_cube_001_dht_ota` | `192.168.1.70` |
| `grova-cube-002` | GROVA PCB v1 | AHT20 temperature/humidity, Bosch BME/BMP pressure | 1 fan | `grova_cube_002_bme_ota` | `192.168.1.97` |

For the current PCB cube, AHT20 is the primary temperature/humidity source. The
Bosch sensor is used for pressure; its temperature reading is exposed as
diagnostic information only.

## Features

- DHT22, AHT20, BME280, and BMP280 sensor support selected by build/profile
  config
- 0.96 inch I2C OLED status display
- Rotary encoder local UI
- Grow modes: germination, growth, harvest
- Automatic and manual light control
- Automatic and manual fan control
- Optional second fan header support
- Fan tachometer support
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

Optional local hardware overrides can be placed in:

```text
include/board_config.h
```

Create it from:

```text
include/board_config.example.h
```

`include/board_config.h` is also ignored by Git. Most users should prefer the
versioned PlatformIO profiles below.

## Build

Build the current deployed cube profiles:

```bash
pio run -e grova_cube_001_dht
pio run -e grova_cube_002_bme
```

Build the local PCB test firmware without MQTT telemetry or commands:

```bash
pio run -e grova_core_v1_local
```

Legacy default build:

```bash
pio run -e esp32dev
```

## Upload

USB upload:

```bash
pio run -e grova_cube_001_dht -t upload
pio run -e grova_cube_002_bme -t upload
```

OTA upload:

```bash
pio run -e grova_cube_001_dht_ota -t upload
pio run -e grova_cube_002_bme_ota -t upload
```

## Hardware Profiles

The built-in defaults are selected with `GROVA_BOARD_PCB_V2`.

Legacy default (`GROVA_BOARD_PCB_V2=0`):

| Function | Default |
| --- | --- |
| Sensor | DHT22 on GPIO 4 |
| OLED | SDA GPIO 19, SCL GPIO 18 |
| Light MOSFET | GPIO 26 |
| Pump MOSFET | GPIO 27 |
| Fan PWM | GPIO 25 |
| Fan tacho | GPIO 35, disabled by default |
| Encoder | CLK GPIO 32, DT GPIO 33, SW GPIO 16 |

GROVA PCB v1 default (`GROVA_BOARD_PCB_V2=1`):

| Function | Default |
| --- | --- |
| Sensor primary | AHT20 on I2C |
| Pressure | BME280/BMP280 on I2C |
| I2C | SDA GPIO 21, SCL GPIO 22 |
| OLED | SSD1306 at `0x3C` |
| Light MOSFET | GPIO 26 |
| Pump MOSFET | GPIO 13 |
| Aux 12 V MOSFET | GPIO 27 |
| Aux 5 V MOSFET | GPIO 14 |
| Fan 1 | PWM GPIO 25, tacho GPIO 34 |
| Fan 2 | PWM GPIO 23, tacho GPIO 35, disabled by default |
| Encoder | CLK GPIO 33, DT GPIO 32, SW GPIO 2 |

## Configuration

Local credentials and cube identity live in:

```text
include/secrets.h
```

Each physical cube needs a unique `MQTT_CUBE_ID`. The provided PlatformIO cube
profiles set this through `GROVA_CUBE_ID_OVERRIDE`.

Persistent runtime settings are stored on the ESP32 through Preferences/NVS:

- grow mode
- light schedule and mode
- pump schedule and duration
- climate day/night targets
- warning limits
- fan curve

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

## MQTT

MQTT is optional. The cube can keep running locally through its display, encoder
UI, schedules, and ESP web fallback even if MQTT is not connected.

Topic shape:

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
```

Telemetry includes `cube_id`, firmware build info, sensor source, fan RPMs,
pressure data where available, warning state, and runtime settings.

## Safety Notes

This is prototype firmware for local grow cube hardware, not production
hardware.

- Verify MOSFET, pump, grow light, wire, connector, and power supply current
  ratings before unattended use.
- Keep flyback protection, fusing, and power distribution under review for every
  board revision.
- A 5 V pump in a 12 V system needs a suitable 5 V supply path.
- Revisit fan tachometer wiring and signal quality before relying on tacho-based
  fault behavior.
- Add electrical protection and safer power distribution before any production
  or long-term unattended setup.

## Documentation

- [Firmware profiles](docs/firmware-profiles.md)
- [API reference](docs/api.md)
- [Display and encoder UI](docs/display-ui.md)
- [Pump safety](docs/pump-safety.md)
- [Changelog](CHANGELOG.md)

## Roadmap

- Add calibration offsets for temperature and humidity.
- Store and expose daily min/max values directly on the cube.
- Improve runtime provisioning for MQTT host, port, cube ID, and credentials.
- Prepare TLS/certificates and per-device credentials for future app or cloud
  operation.
- Continue hardening electrical protection and board-level power distribution.

## Repository Scope

This repository intentionally contains only firmware-relevant files.

Included:

- `src/`
- `include/`
- `lib/`
- `test/`
- `docs/`
- `platformio.ini`
- `include/secrets.example.h`
- `include/board_config.example.h`

Excluded:

- real secrets in `include/secrets.h`
- local hardware overrides in `include/board_config.h`
- server, dashboard, Grafana, InfluxDB, Node-RED, and deployment files
- local databases, logs, backups, and `.env` files

## ⚖️ License

This firmware is open-source software licensed under the terms of the **GNU General Public License v3 (GPLv3)**. 

You are free to use, copy, modify, and distribute this software, provided that any modified versions or derivative works are also openly published under the exact same GPLv3 license terms. For the full legal text, please refer to the accompanying [LICENSE](LICENSE) file in the root directory.
