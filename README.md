<div align="center">

# GROVA CORE

## Professional Climate Control.
### Open by Design.

An open-source environmental automation controller designed for Home Assistant and ESPHome.

Built for builders, makers and professionals who demand reliable climate control without cloud dependency, subscriptions or vendor lock-in.



🚀 Local-First <br>
🔄 OTA Updates <br>
📡 MQTT Connectivity <br>
⚡ Integrated Power Distribution <br>

👉 Interested in the first 50 developer kits?
<br>

?? [GROVAHOME](https://grova.carrd.co)

?? Batch #01 Waitlist Open

</div>

---

# Why GROVA?

GROVA CORE combines climate monitoring, ventilation control, sensor integration and automation into a single platform.

Designed around a local-first philosophy, it keeps your infrastructure running independently of cloud services, external APIs or internet connectivity.

### Key Benefits

- ? Fully Local Operation
- ? ESP32 Based
- ? MQTT Support
- ? REST API Support
- ? Designed For Local Automation
- ? Home Assistant Integration Planned
- ? Open Source Firmware
- ? Open Hardware Architecture
- ? Dual PWM Fan Control
- ? High-Power MOSFET Outputs
- ? Sensor Expansion Ready
- ? No Recurring Fees
- ? No Vendor Lock-In
- ? Swiss Engineered

---

# Built For Real-World Environments

## Smart Buildings

Monitor and automate environmental conditions in server rooms, utility spaces, technical infrastructure and critical installations.

## Greenhouse Automation

Maintain stable climate conditions with automated ventilation, humidity management and environmental monitoring.

## Terrariums & Habitats

Control lighting, ventilation, misting systems and environmental sensors from a single controller.

## Research & Development

Reliable environmental monitoring and automation for experiments, prototypes and controlled environments.

## Indoor Agriculture

Flexible climate automation for controlled growing environments and precision cultivation systems.

---

# Core Capabilities

## Environmental Monitoring

Connect temperature, humidity, pressure and environmental sensors through GROVA's modular sensor architecture.

## Precision Ventilation Control

Automatically regulate airflow based on environmental conditions or custom automation logic.

## High-Power Load Control

Drive fans, pumps, lighting, valves and heaters using dedicated MOSFET outputs.

## Automation Profiles

Create autonomous schedules and environmental control routines.

## Fully Local Architecture

All core functionality continues operating without internet access, cloud services or central servers.

---

# Batch #01

## First Community Release

The first production run will consist of only **50 developer kits**.

Every Batch #01 kit includes:

- GROVA CORE Baseboard
- ESP32 Development Module
- Open Source Firmware
- Early Community Access
- Priority Development Updates
- STL Enclosure Files

### Join The Waitlist

?? [GROVAHOME](https://grova.carrd.co)

---
# Recent Changes and Updates

Offline grow mode is now supported through ESP-local preset storage: each board
can keep up to 5 local presets, with up to 10 phases per preset and up to 5 pump
events per phase. After a run is started, the ESP can continue applying phase
targets and pump events without the server being online.

Planned hardware direction: the legacy DHT cube remains supported until Cube 001
is replaced by a second GROVA PCB v1 build. 

# Hardware Overview

The current alpha revision is based on the GROVA CORE Baseboard.

## Supported Hardware Features

### Power Outputs

- 2x High-Power 12V MOSFET Outputs
- 2x High-Power 5V MOSFET Outputs

### Fan Control

- 2x Independent PWM Fan Outputs
- Fan Tachometer Support

### Sensor Expansion

- 4x Buffered I2C Channels
- 3.3V / 5V Compatible

### Interfaces

- OLED Display Support
- Rotary Encoder Support
- Local Web Interface
- OTA Updates

### Planned PCB v1 Expansion Headers

| Header | Purpose | Pins |
| --- | --- | --- |
| J1 | Analog input 1 | 3.3V, GND, GPIO36 |
| J2 | Analog input 2 | 3.3V, GND, GPIO39 |
| J3 | One-Wire / digital | 3.3V, GND, GPIO4 |
| J4 | UART / expansion | 5V, 3.3V, GND, TX17, RX16 |
| J5 | Pulse / flow | 5V, 3.3V, GND, GPIO18 |
| J6 | Digital safety input | 3.3V, GND, GPIO19 |

Reserved pins: GPIO0 for BOOT/recovery, GPIO5 and GPIO15 as internal reserve,
and GPIO12 unused. ESP32 signal pins are not 5V tolerant; 5V header pins are
power rails only. GPIO36/GPIO39 are ADC1 input-only pins without internal
pullups/pulldowns. 5V pulse outputs need level shifting or open-collector
wiring with a 3.3V pullup.

### Control Hardware

- ESP32 Compatible
- Optional DS3231/DS1307-compatible RTC Support

---

# Firmware Features

- Environmental Monitoring
- Automated Fan Control
- Manual Fan Control
- Automated Light Control
- Manual Light Control
- Pump Scheduling
- Pump Safety Protection
- OLED Status Interface
- Rotary Encoder Interface
- OTA Firmware Updates
- MQTT Telemetry
- Versioned HTTP APIs
- Persistent Runtime Storage
- Optional RTC Time Fallback
- Persistent Native Rest Mode
- Local Offline Preset Mode, up to 5 ESP presets
- Offline Grow Run Engine

---

# Architecture

GROVA CORE follows a local-first architecture.

All environmental monitoring, automation logic and hardware control run locally on the controller.

MQTT connectivity is optional and can be enabled for:

- Remote Monitoring
- Historical Data Logging
- Multi-Device Management
- Centralized Dashboards

The controller remains fully operational without MQTT or internet connectivity.

---

# Supported Sensors

Current firmware supports:

- AHT20
- DHT22
- BME280
- BMP280
- Optional DS3231/DS1307-compatible RTC at `0x68`

Additional I2C-based sensors can be added through the expansion architecture.

---

# Supported Interfaces

## Outputs

- MOSFET Outputs
- PWM Fan Outputs

## Inputs

- Environmental Sensors
- Rotary Encoder
- Fan Tachometers

## Connectivity

- Wi-Fi
- MQTT
- HTTP API
- OTA Updates

---

# Current Baseline

| Component | Value |
|-----------|--------|
| Board | GROVA CORE v1 |
| MCU | ESP32 DevKit Compatible |
| Framework | Arduino |
| Build System | PlatformIO |
| Firmware Branch | grova-core-v1 |
| Operation | Local First |
| Optional RTC | DS3231/DS1307-compatible I2C RTC at `0x68`, disabled by default |

---

# Setup

Copy the example secrets file:

```bash
cp include/secrets.example.h include/secrets.h
```

Fill in your credentials and configuration values.

The file is excluded from version control.

---

# Build

Local development build:

```bash
pio run -e grova_core_v1_local
```

Example production builds:

```bash
pio run -e grova_cube_001_dht
pio run -e grova_cube_002_bme
```

---

# Upload

USB Upload

```bash
pio run -e grova_cube_001_dht -t upload
```

OTA Upload

```bash
pio run -e grova_cube_001_dht_ota -t upload
```

---

# Active Cube Profiles

| Cube ID | Hardware | Sensors | Fan | OTA target | OTA IP |
| --- | --- | --- | --- | --- | --- |
| `grova-cube-001` | Legacy cube | DHT22 temperature/humidity | 1 fan | `grova_cube_001_dht_ota` | `192.168.1.70` |
| `grova-cube-002` | GROVA PCB v1 | AHT20 temperature/humidity, Bosch BME/BMP pressure | 2 independent PWM fan outputs | `grova_cube_002_bme_ota` | `192.168.1.97` |

---

# Hardware Profiles

The built-in defaults are selected with `GROVA_BOARD_PCB_V1`.

Legacy default (`GROVA_BOARD_PCB_V1=0`):

| Function | Default |
| --- | --- |
| Sensor | DHT22 on GPIO 4 |
| OLED | SDA GPIO 19, SCL GPIO 18 |
| Light MOSFET | GPIO 26 |
| Pump MOSFET | GPIO 27 |
| Fan PWM | GPIO 25 |
| Fan tacho | GPIO 35, disabled by default |
| Encoder | CLK GPIO 32, DT GPIO 33, SW GPIO 16 |
| Optional RTC | DS3231/DS1307-compatible I2C RTC at `0x68`, disabled by default |

GROVA PCB v1 default (`GROVA_BOARD_PCB_V1=1`):

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
| Fan 2 | PWM GPIO 23 enabled by default, tacho GPIO 35 optional |
| Encoder | CLK GPIO 33, DT GPIO 32, SW GPIO 2 |
| Optional RTC | DS3231/DS1307-compatible I2C RTC at `0x68`, disabled by default |

Planned PCB v1 expansion map:

| Header | Purpose | Pins |
| --- | --- | --- |
| J1 | Analog input 1 | 3.3V, GND, GPIO36 |
| J2 | Analog input 2 | 3.3V, GND, GPIO39 |
| J3 | One-Wire / digital | 3.3V, GND, GPIO4 |
| J4 | UART / expansion | 5V, 3.3V, GND, TX17, RX16 |
| J5 | Pulse / flow | 5V, 3.3V, GND, GPIO18 |
| J6 | Digital safety input | 3.3V, GND, GPIO19 |

Reserved: GPIO0 BOOT/recovery, GPIO5 internal reserve, GPIO12 do not use,
GPIO15 internal reserve.

Older local `board_config.h` files that still define `GROVA_BOARD_PCB_V2` are
accepted as a backwards-compatible alias, but new configs should use
`GROVA_BOARD_PCB_V1`.

---

# Configuration

Local credentials and cube identity live in:

```text
include/secrets.h
```

Persistent runtime settings are stored on the ESP32 through Preferences/NVS:

- grow mode
- light schedule and mode
- pump schedule and duration
- climate day/night targets
- warning limits
- fan curve
- optional RTC enablement
- local preset slots and active local run state

Optional MQTT/Home Assistant settings in `include/secrets.h`:

```cpp
#define MQTT_DEVICE_NAME "GROVA Cube 1"
#define MQTT_DISCOVERY_PREFIX "homeassistant"
#define GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED 1
```

---

# Local APIs

The cube exposes versioned local HTTP APIs when connected to Wi-Fi:

```text
GET  /api/v1/status
POST /api/v1/control
GET  /api/v1/config
POST /api/v1/config
GET  /api/v1/local-presets
```

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

If `fan` is omitted, fan commands apply to all enabled fan channels for backward
compatibility.

```json
{"cmd":"set_fan_auto"}
```


---

# Native Rest Mode

Rest Mode is a persistent firmware state for parking a cube between grows without overwriting the stored grow mode or manual settings.

```json
{"cmd":"set_rest_mode","enabled":true}
```

```json
{"cmd":"set_rest_mode","enabled":false}
```

When enabled, the firmware forces the light output off, forces all fan PWM outputs to 0%, stops and blocks pump runs, and suspends temperature/humidity warning limits. The enabled flag is stored in ESP32 Preferences/NVS and is restored after reboot only when Rest Mode was explicitly enabled before restart. Status and MQTT telemetry expose `rest_mode.enabled`, `rest_mode.mode`, and `rest_mode.reason`.

---

# Local Offline Preset Mode

The firmware can store grow presets directly on the ESP32 and execute one active grow locally. This lets the cube keep running a started grow even if the server, MQTT bridge or dashboard is offline.

Limits:

```text
Local ESP preset slots: 5
Maximum phases per preset: 10
Maximum pump events per phase: 5
Active local run: 1
```

The server/dashboard can store more profiles, but only selected profiles are synced into the ESP's five local slots for offline execution.

Example control commands:

```json
{"cmd":"set_local_preset","slot":0,"payload_hex":"..."}
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

During a local run, the ESP applies phase grow mode, climate targets, light schedule and due pump events itself. Server-side stop, pause/resume commands and active-preset resyncs can still intervene when the cube is online. Pump safety uses an event lock instead of a hard daily/interval throttle: the same scheduled date/minute event cannot retrigger, while the 10 s maximum runtime, startup lock, missing time sync, harvest block and Rest Mode block remain active.

---
# Optional RTC Support

The firmware supports DS3231/DS1307-compatible RTC modules on the shared I2C bus at address `0x68`.

RTC support is intentionally disabled by default so cubes without RTC hardware keep behaving exactly like NTP-only devices. Enable it only when an RTC module is fitted.

```json
{"cmd":"set_rtc_config","enabled":true}
```

```json
{"cmd":"set_rtc_config","enabled":false}
```

When enabled and valid, the RTC can seed ESP system time during boot. NTP remains the primary online time source and refreshes the RTC later. Status and MQTT telemetry expose `time_source` plus `rtc.enabled`, `rtc.present`, `rtc.valid`, `rtc.used_for_boot`, `rtc.last_read_ok`, and `rtc.last_write_ok`.

Current prototype note: Cube 001 and Cube 002 do not have RTC hardware fitted yet, so both are deployed with `rtc.enabled=false`.
---

# MQTT Topics

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
```

MQTT remains optional.

All critical functionality runs locally even without connectivity.

---

# Roadmap

## GROVA CORE v1

- Community Validation
- First Developer Batch
- Firmware Stabilization
- Planned expansion headers for analog inputs, One-Wire/digital, UART, pulse/flow and safety input
- Keep legacy DHT profile only until Cube 001 is replaced by a second PCB v1 build

## GROVA CORE v2

- Integrated ESP32
- Integrated RTC
- Simplified Assembly
- Optimized Power Distribution
- Reduced BOM Complexity

## Future Ecosystem

- Environmental Sensor Modules
- Air Quality Extensions
- DIN Rail Variant
- Additional Expansion Boards
- Advanced Monitoring Accessories

---

# Documentation

Available inside the `/docs` directory:

- Firmware Profiles
- API Documentation
- Display Interface
- Pump Safety Documentation
- Changelog

---

# Repository Scope

Included:

```text
src/
include/
lib/
test/
docs/
platformio.ini
```

Excluded:

```text
Secrets
Local Overrides
Deployment Files
Databases
Logs
Backups
```

---

# License

GNU GPLv3

This project is released under the GNU General Public License Version 3.

See the LICENSE file for further details.

---

<div align="center">

## Own Your Infrastructure.

No cloud dependency.

No vendor lock-in.

No recurring fees.

Just reliable climate control.

?? https://grovahome.com

</div>
