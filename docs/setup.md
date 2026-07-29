# GROVA CORE Setup Guide

This guide covers the basic local setup for building and flashing the standalone GROVA CORE firmware.

For the product overview, see the main [README](../README.md). For APIs, MQTT topics and runtime behavior, see [technical.md](technical.md).

## Requirements

- PlatformIO Core or PlatformIO IDE
- A supported ESP32 or ESP32-S3 board
- USB cable for first flashing
- Wi-Fi credentials for network features
- Optional MQTT broker for telemetry and remote commands

## Repository Setup

Clone the firmware repository:

```bash
git clone https://github.com/grovahome/grova-cube-firmware.git
cd grova-cube-firmware
```

Copy the example secrets file:

```bash
cp include/secrets.example.h include/secrets.h
```

Fill in your local Wi-Fi, MQTT and cube identity values in `include/secrets.h`.

Do not commit real credentials. `include/secrets.h` is intentionally excluded from version control.

## Cube Identity

Each physical cube should use a unique MQTT cube ID and device name.

Typical local overrides in `include/secrets.h`:

```cpp
#define MQTT_CUBE_ID "grova-cube-001"
#define MQTT_DEVICE_NAME "GROVA Cube 1"
#define MQTT_DISCOVERY_PREFIX "homeassistant"
#define GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED 1
```

## Build Profiles

Current example profiles:

| Environment | Purpose |
| --- | --- |
| `grova_core_v1_local` | Local test build without production cube targeting |
| `grova_cube_001_dht` | Legacy DHT cube build |
| `grova_cube_001_dht_ota` | Legacy DHT cube OTA upload |
| `grova_cube_002_bme` | GROVA PCB v1 build with AHT20 and Bosch pressure sensor |
| `grova_cube_002_bme_ota` | GROVA PCB v1 OTA upload |

See [firmware-profiles.md](firmware-profiles.md) for detailed hardware profile notes.

## Build

Local development build:

```bash
pio run -e grova_core_v1_local
```

Example cube builds:

```bash
pio run -e grova_cube_001_dht
pio run -e grova_cube_002_bme
```

## USB Upload

Example USB upload:

```bash
pio run -e grova_cube_001_dht -t upload
```

Use the environment that matches your board and local configuration.

## OTA Upload

After the first USB flash and network setup, OTA upload can be used when the cube is reachable on the network.

```bash
pio run -e grova_cube_001_dht_ota -t upload
pio run -e grova_cube_002_bme_ota -t upload
```

## Local API Check

When the cube is connected to Wi-Fi, the status endpoint is available at:

```text
http://<cube-ip>/api/v1/status
```

The older alias also exists:

```text
http://<cube-ip>/api/status
```

## Runtime Configuration

Persistent runtime settings are stored on the ESP32 through Preferences/NVS, including:

- grow mode
- light schedule and mode
- pump schedule and duration
- climate day/night targets
- warning limits
- fan curve
- optional RTC enablement
- local preset slots
- active local run state
- native Rest Mode state

## Safety Notes

- Do not commit real Wi-Fi or MQTT credentials.
- Keep all ESP32 GPIO signals at 3.3 V logic level.
- 5 V header pins are power rails only unless proper level shifting or open-collector wiring is used.
- Automatic pump runs are protected by firmware safety limits, but hardware loads should still be wired and fused appropriately.

## More Documentation

- [Technical documentation](technical.md)
- [HTTP and MQTT API](api.md)
- [Firmware profiles](firmware-profiles.md)
- [Display and encoder UI](display-ui.md)
- [Pump safety](pump-safety.md)
- [Hardware reference](hardware/README.md)
