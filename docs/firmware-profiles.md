# GROVA Firmware Profiles

Status date: 2026-07-25

The firmware uses one software line for all active cubes. The legacy DHT cube
and the GROVA PCB v1 cube are built from the same source code; only the
hardware profile changes through PlatformIO build flags or an optional local
`include/board_config.h`.

Credentials stay local in `include/secrets.h`. Each cube must set its own
`MQTT_CUBE_ID`/device name, but no separate firmware version or branch is
needed for the old PCB/wiring.

RTC support is included in the shared firmware line, but it is disabled by
default because the current active cubes do not have RTC modules fitted yet.

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
  Fan:      two independent PWM fan outputs, Fan 2 tacho optional
  Outputs:  LED, pump, aux 12V, aux 5V MOSFETs
  OTA IP:   192.168.1.97
  Build:    grova_cube_002_bme
  OTA:      grova_cube_002_bme_ota
```

The cube ID is part of MQTT topics, payloads, ACK matching, dashboard state,
history, and per-cube commands. Each physical cube must use a unique ID.

## Planned PCB v1 Expansion Headers

The old DHT cube is expected to be replaced by a second GROVA PCB v1 build.
After that transition, the legacy DHT wiring profile can be treated as
deprecated and the PCB v1 expansion map can become the normal hardware target.

Planned additional headers for GROVA Core PCB v1:

```text
J1 - ANALOG INPUT 1
  3.3V | GND | GPIO36

J2 - ANALOG INPUT 2
  3.3V | GND | GPIO39

J3 - ONE-WIRE / DIGITAL
  3.3V | GND | GPIO4

J4 - UART / EXPANSION
  5V | 3.3V | GND | TX17 | RX16

J5 - PULSE / FLOW
  5V | 3.3V | GND | GPIO18

J6 - DIGITAL SAFETY INPUT
  3.3V | GND | GPIO19
```

Reserved pins:

```text
GPIO0  -> BOOT/Recovery
GPIO5  -> internal reserve
GPIO12 -> do not use
GPIO15 -> internal reserve
```

Hardware notes:

```text
ESP32 GPIOs are not 5V tolerant; 5V headers are power only, signal inputs must stay at 3.3V.
GPIO36 and GPIO39 are ADC1 input-only pins and have no internal pullups/pulldowns.
Pulse/flow sensors that output 5V need level shifting or open-collector wiring with a 3.3V pullup.
The safety input should use external pullup/pulldown and fail-safe wiring so cable faults can be detected later.
```

## Optional RTC

The firmware supports an optional DS3231/DS1307-compatible RTC on the existing
I2C bus.

```text
I2C address: 0x68
Default: disabled
Compile-time default: GROVA_RTC_DEFAULT_ENABLED=0
Persistent runtime setting: ESP32 Preferences/NVS key rtc_enabled
Current Cube 001: no RTC fitted, rtc.enabled false
Current Cube 002: no RTC fitted, rtc.enabled false
```

When enabled and a valid RTC is present, the cube can seed ESP system time from
RTC at boot. NTP remains active and is still the primary online time source; it
refreshes the RTC after the system has a valid network time.

Runtime control through HTTP or MQTT:

```json
{"cmd":"set_rtc_config","enabled":true}
```

```json
{"cmd":"set_rtc_config","enabled":false}
```

Status and MQTT telemetry expose:

```text
time_source
rtc.enabled
rtc.present
rtc.valid
rtc.used_for_boot
rtc.last_read_ok
rtc.last_write_ok
```

## Native Rest Mode

Rest Mode is a persistent firmware state for parking a cube between grows
without overwriting the stored grow mode or manual settings.

Runtime control through HTTP or MQTT:

```json
{"cmd":"set_rest_mode","enabled":true}
```

```json
{"cmd":"set_rest_mode","enabled":false}
```

When enabled:

```text
Light output is forced off.
Fan 1 and Fan 2 targets/PWM are forced to 0%.
Automatic pump runs and pump tests are blocked.
Any running pump is stopped.
Temperature and humidity warning limits are suspended.
The current grow mode and manual settings remain stored.
```

The enabled flag is stored in ESP32 Preferences/NVS and is loaded on boot. A
restart only re-enters Rest Mode when Rest Mode was explicitly enabled before
the restart.

Status and MQTT telemetry expose:

```text
rest_mode.enabled
rest_mode.mode
rest_mode.reason
```

## Local Offline Presets

The firmware stores compact grow presets in ESP32 Preferences/NVS and can run one active grow locally after the server starts it.

Limits:

```text
Local ESP preset slots: 5
Maximum phases per preset: 10
Maximum pump events per phase: 5
Active local run: 1
```

Runtime control through HTTP or MQTT:

```json
{"cmd":"set_local_preset","slot":0,"payload_hex":"..."}
```

```json
{"cmd":"start_local_run","slot":0,"start_at_ms":1780000000000,"run_id":"run-...","revision":123456}
```

```json
{"cmd":"pause_local_run"}
```

```json
{"cmd":"resume_local_run"}
```

```json
{"cmd":"stop_local_run"}
```

Status and MQTT telemetry expose `local_presets` and `local_run`, including `local_run.status`, `phase_label`, progress fields and `paused_at_s`. During local execution the ESP applies phase grow mode, climate targets, light schedule and due pump events without needing the server to stay online. Pause/resume is persisted locally and resume shifts the effective start time so phase progress does not advance while paused. Pump safety uses an event lock instead of a daily/interval throttle.

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

Latest verified RTC-support OTA uploads:

```text
2026-07-25 grova_cube_001_dht_ota -> 192.168.1.70: success, warning OK, rtc.enabled false
2026-07-25 grova_cube_002_bme_ota -> 192.168.1.97: success, warning OK, rtc.enabled false
```

Latest verified native Rest Mode OTA uploads:

```text
2026-07-25 grova_cube_001_dht_ota -> 192.168.1.70: success, healthy true, warning OK, rest_mode.enabled true after server enforcement
2026-07-25 grova_cube_002_bme_ota -> 192.168.1.97: success, healthy true, warning OK, rest_mode.enabled true after server enforcement
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
  Versioned example for PCB v1 and legacy wiring profiles.
  Includes optional RTC defaults: RTC_I2C_ADDR 0x68 and GROVA_RTC_DEFAULT_ENABLED 0.
```

Do not commit real Wi-Fi or MQTT credentials.

Use `GROVA_BOARD_PCB_V1=1` for the GROVA PCB v1 defaults and
`GROVA_BOARD_PCB_V1=0` for the legacy DHT wiring defaults. Older local configs
that still define `GROVA_BOARD_PCB_V2` continue to work as an alias.

## MQTT Topics

```text
grova/v1/cubes/{cube_id}/telemetry
grova/v1/cubes/{cube_id}/command
grova/v1/cubes/{cube_id}/ack
grova/v1/cubes/{cube_id}/availability
```

The dashboard subscribes to all cubes and sends commands only to the selected
cube ID.

## Home Assistant MQTT Discovery

Home Assistant discovery is enabled by default when MQTT is enabled. The cube
publishes one retained device discovery payload and groups all entities under
one Home Assistant device.

Discovery topic:

```text
homeassistant/device/{cube_id}/config
```

Availability topic:

```text
grova/v1/cubes/{cube_id}/availability
```

The availability payload is retained:

```text
online
offline
```

The MQTT Last Will is set to `offline`. On connect, the cube publishes
`online`, republishes the retained discovery payload, and sends telemetry. The
cube also subscribes to:

```text
homeassistant/status
```

When Home Assistant publishes `online`, the cube republishes discovery and
telemetry so entities become available after a Home Assistant restart.

Discovery payload root fields:

```json
{
  "device": {
    "identifiers": ["grova_<cube_id>"],
    "name": "<MQTT_DEVICE_NAME>",
    "manufacturer": "GROVA",
    "model": "GROVA Core Founder Edition",
    "serial_number": "<cube_id>",
    "hw_version": "<GROVA_HARDWARE_VERSION>",
    "sw_version": "<GROVA_FIRMWARE_VERSION>",
    "configuration_url": "http://<cube-ip>"
  },
  "origin": {
    "name": "GROVA Core Firmware",
    "sw_version": "<GROVA_FIRMWARE_VERSION>",
    "support_url": "https://github.com/grovahome/grova-cube-firmware"
  },
  "components": {}
}
```

Initial discovered components:

```text
Sensors:
  Temperature
  Humidity
  Pressure
  Fan speed
  Fan RPM
  Pump runs today

Binary sensors:
  Healthy
  Pump running

Controls:
  Light
  Fan 1 speed number slider
  Fan 1 auto button
  Fan 2 speed number slider, when FAN2_ENABLED is set
  Fan 2 auto button, when FAN2_ENABLED is set
  12V output switch, when PIN_AUX_12V is available
  5V output switch, when PIN_AUX_5V is available
  Pump test button
  Stop pump button
```

MQTT commands for independent fan control:

```json
{"cmd":"set_fan_manual","fan":1,"percent":40}
```

```json
{"cmd":"set_fan_manual","fan":2,"percent":70}
```

```json
{"cmd":"set_fan_auto","fan":1}
```

```json
{"cmd":"set_fan_auto","fan":2}
```

If `fan` is omitted or set to `0`, the command applies to all enabled fan
channels for backward compatibility with the existing dashboard.

MQTT commands for auxiliary MOSFET outputs:

```json
{"cmd":"set_output","output":"aux_12v","state":true}
```

```json
{"cmd":"set_output","output":"aux_5v","state":false}
```

Optional local overrides in `include/secrets.h`:

```cpp
#define MQTT_DEVICE_NAME "GROVA Cube 1"
#define MQTT_DISCOVERY_PREFIX "homeassistant"
#define GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED 1
#define GROVA_SUPPORT_URL "https://github.com/grovahome/grova-cube-firmware"
```

`GROVA_HARDWARE_VERSION` defaults to `Legacy wiring` for the DHT profile and
`GROVA PCB v1` for the PCB profile. It can be overridden in
`include/board_config.h` if needed.

For PCB v1, `FAN2_ENABLED` defaults to `1` so GPIO 23 is available as a second
independent PWM output. `FAN2_TACHO_ENABLED` defaults to `0` to avoid warnings
when no second RPM wire is connected yet. Enable it locally only when Fan 2 RPM
is wired and should be monitored.
