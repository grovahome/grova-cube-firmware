<div align="center">

# GROVA CORE

## Professional Climate Control.
### Open by Design.

An open-source environmental automation controller designed for Home Assistant and ESPHome.

Built for builders, makers and professionals who demand reliable climate control without cloud dependency, subscriptions or vendor lock-in.

<br>

🌐 [https://grovahome.com](https://grova.carrd.co)

🚀 Batch #01 Waitlist Open

</div>

---

# Why GROVA?

GROVA CORE combines climate monitoring, ventilation control, sensor integration and automation into a single platform.

Designed around a local-first philosophy, it keeps your infrastructure running independently of cloud services, external APIs or internet connectivity.

### Key Benefits

- ✅ Fully Local Operation
- ✅ ESP32 Based
- ✅ MQTT Support
- ✅ REST API Support
- ✅ Designed For Local Automation
- ✅ Home Assistant Integration Planned
- ✅ Open Source Firmware
- ✅ Open Hardware Architecture
- ✅ Dual PWM Fan Control
- ✅ High-Power MOSFET Outputs
- ✅ Sensor Expansion Ready
- ✅ No Recurring Fees
- ✅ No Vendor Lock-In
- ✅ Swiss Engineered

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

👉 [https://grovahome.com](https://grova.carrd.co)

---

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

### Control Hardware

- ESP32 Compatible
- DS3231 RTC Support

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

# Local APIs

Status

```http
GET /api/v1/status
```

Configuration

```http
GET /api/v1/config
POST /api/v1/config
```

Control

```http
POST /api/v1/control
```

Example

```json
{
  "cmd":"set_fan_manual",
  "percent":50
}
```

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

🌐 https://grovahome.com

</div>
