# GROVA Firmware Profiles

Status date: 2026-07-31

The active firmware line now targets only the current GROVA PCB v1 / Founder
Edition hardware. Legacy hand-wired DHT hardware is frozen and is no longer
updated from this source line.

Credentials stay local in `include/secrets.h`. Each cube must set its own
`MQTT_CUBE_ID`/device name.

Legacy DHT hardware remains documented for traceability only:

```text
Last commit intended to support legacy DHT hardware: 11eacaf Add optional SHT41 sensor support
Last stable release tag remains: v1.1.0 -> 0917d42
Current source line after this cleanup: PCB v1 / Founder Edition only
```

RTC support is included in the shared firmware line, but it is disabled by
default because the current active cubes do not have RTC modules fitted yet.

SHT41/SHT4x temperature/humidity support is compiled into PCB/Founder builds
by default and remains optional at runtime. If the sensor is connected and
initializes successfully, it becomes the active temperature/humidity source.

## Active Hardware Target

```text
  Hardware: GROVA PCB v1
  Sensor:   optional I2C modules, auto-detected at boot/restart and periodic rescan
  Fan:      two independent PWM fan outputs, Fan 2 tacho optional
  Outputs:  LED, pump, aux 12V, aux 5V MOSFETs
  Default:  no sensor is required
  Build:    grova_core_v1
```

The cube ID is part of MQTT topics, payloads, ACK matching, dashboard state,
history, and per-cube commands. Each physical cube must use a unique ID.

## PCB v1 Expansion Headers

Additional headers for GROVA Core PCB v1:

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
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_core_v1
```

OTA upload uses the firmware binary from the same single profile. Pass the
current cube IP at upload time so no local LAN address is committed:

```powershell
C:\Users\Kai\.platformio\penv\Scripts\platformio.exe run -e grova_core_v1
C:\Users\Kai\.platformio\penv\Scripts\python.exe C:\Users\Kai\.platformio\packages\framework-arduinoespressif32\tools\espota.py -i <cube-ip> -p 3232 -f .pio\build\grova_core_v1\firmware.bin
```

Do not run firmware updates against legacy hardware from this source line.

## Sensor Roles

The sensor module reads the enabled primary temperature/humidity source in this
order:

```text
1. SHT41/SHT4x, when compiled in and detected
2. SCD41, when detected and a measurement is available
3. BME280 humidity/temperature fallback, when Bosch is enabled and present
```

For PCB/Founder builds, all planned I2C sensor families are compiled in by
default. Missing sensors are normal. The cube reports unavailable channels as
`null` and continues with the available modules.

Independent optional channels:

```text
Pressure: BME280/BMP280
CO2:      SCD41
Lux:      VEML7700
UV index: LTR390
RTC:      DS3231/DS1307-compatible module at 0x68
```

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
  Versioned example for the active PCB v1 / Founder Edition profile.
  Includes optional RTC defaults and runtime I2C sensor detection.
```

Do not commit real Wi-Fi or MQTT credentials.

Use the PCB v1 defaults for new work. Legacy hardware should stay on the frozen
commit listed at the top of this document.

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
  CO2
  Light / lux
  UV index
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

`GROVA_HARDWARE_VERSION` defaults to `GROVA PCB v1`. It can be overridden in
`include/board_config.h` if needed for a prototype batch.

For PCB v1, `FAN2_ENABLED` defaults to `1` so GPIO 23 is available as a second
independent PWM output. `FAN2_TACHO_ENABLED` defaults to `0` to avoid warnings
when no second RPM wire is connected yet. Enable it locally only when Fan 2 RPM
is wired and should be monitored.
