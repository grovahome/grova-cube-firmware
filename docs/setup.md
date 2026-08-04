# GROVA CORE Setup Guide

This guide covers the basic local setup for building and flashing the standalone GROVA CORE firmware.

For the product overview, see the main [README](../README.md). For APIs, MQTT topics and runtime behavior, see [technical.md](technical.md).

## Requirements

- PlatformIO Core or PlatformIO IDE
- A supported ESP32 or ESP32-S3 board
- USB cable for first flashing
- Wi-Fi credentials for network features
- Optional MQTT broker for telemetry and remote commands
- Optional GROVA I2C sensor modules: SHT41/SHT4x, AHT20/AHTx0, VEML7700, SCD41, BME/BMP, LTR390

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

## Optional I2C Sensors

The active firmware target is GROVA PCB v1 / Founder Edition. All planned I2C
sensor families are compiled in by default and remain optional at runtime. If a
known sensor is connected on the I2C bus and initializes successfully, the cube
uses it without user code changes.

Temperature and humidity priority:

```text
SHT41/SHT4x -> AHT20/AHTx0 -> SCD41 -> BME280
```

Missing sensors are normal. Their telemetry channels are emitted as `null` and
must not create firmware errors.

## Build Profiles

Current profile:

| Environment | Purpose |
| --- | --- |
| `grova_core_v1` | Current PCB v1 / Founder Edition build |

See [firmware-profiles.md](firmware-profiles.md) for detailed hardware profile notes.

## Build

Local development build:

```bash
pio run -e grova_core_v1
```

## USB First Flash

The active profile is configured for OTA updates. For a first USB flash, use a
local temporary PlatformIO setting or change `upload_protocol` to `esptool`
locally, flash once, and then return to the committed OTA default.

## OTA Upload

After the first USB flash and network setup, build the single profile and upload
the resulting firmware binary with Espressif OTA:

```powershell
C:\Users\Kai\.platformio\penv\Scripts\pio.exe run -e grova_core_v1
C:\Users\Kai\.platformio\penv\Scripts\python.exe C:\Users\Kai\.platformio\packages\framework-arduinoespressif32\tools\espota.py -i <cube-ip> -p 3232 -f .pio\build\grova_core_v1\firmware.bin
```

Before OTA, verify that the local ignored `include/secrets.h` exists and
contains the correct Wi-Fi credentials. A build without local secrets will boot
without Wi-Fi and cannot be recovered over OTA.

Do not run firmware updates against the frozen legacy DHT cube from this source
line.

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
