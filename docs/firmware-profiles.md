# GROVA Firmware Profiles

Status date: 2026-07-21

The firmware now supports multiple cube hardware profiles from one codebase.
Credentials stay local in `include/secrets.h`. Hardware differences are selected
through PlatformIO build flags or an optional local `include/board_config.h`.

## Active Cubes

```text
grova-cube-001
  Hardware: legacy cube
  Sensor:   DHT22 on DHTPIN
  Fan:      one fan
  OTA IP:   192.168.1.70
  Build:    grova_cube_001_dht
  OTA:      grova_cube_001_dht_ota

grova-cube-002
  Hardware: GROVA PCB v1
  Sensor:   AHT20 for temperature/humidity, Bosch BME/BMP for pressure
  Fan:      one fan
  OTA IP:   192.168.1.97
  Build:    grova_cube_002_bme
  OTA:      grova_cube_002_bme_ota
```

The cube ID is part of MQTT topics, payloads, ACK matching, dashboard state,
history, and per-cube commands. Each physical cube must use a unique ID.

## Build Commands

```powershell
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_cube_001_dht
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_cube_002_bme
```

OTA upload:

```powershell
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_cube_001_dht_ota -t upload
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_cube_002_bme_ota -t upload
```

Local test build for the new PCB, without MQTT telemetry or commands:

```powershell
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_core_v1_local
```

## Sensor Roles

The sensor module reads the enabled primary temperature/humidity source in this
order:

```text
1. AHT20, when enabled and detected
2. DHT, when enabled
3. BME280 humidity/temperature fallback, when Bosch is enabled and no primary source exists
```

For the current PCB cube, AHT20 is the primary temperature/humidity sensor. The
Bosch sensor is used for pressure. Its temperature reading is exposed only as
diagnostic information.

Transient sensor read failures are debounced. A single missed read keeps the
last valid measurement and does not immediately create a cube warning. A warning
is raised after `SENSOR_READ_FAIL_WARN_AFTER` consecutive failed reads.

## Config Files

```text
include/secrets.h
  Local credentials and MQTT settings. Ignored by Git.

include/board_config.h
  Optional local hardware profile override. Ignored by Git.

include/board_config.example.h
  Versioned example for PCB and legacy wiring profiles.
```

Do not commit real Wi-Fi or MQTT credentials.

## MQTT Topics

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
```

The dashboard subscribes to all cubes and sends commands only to the selected
cube ID.
