# GROVA Core ESP32-S3 hardware

Current status (16 November 2026): the integrated S3 application is installed;
Wi-Fi/OTA, SHT4x measurements, mux and RV-3032 operation have been checked.
Electrical output/load, long-term RTC, display/encoder and power/recovery acceptance
remain open. See the [bring-up record](../../board-bringup.md).

The dated initial-target notes below are historical; statements about missing
application drivers or no hardware installation describe that earlier checkpoint.

## Initial hardware baseline

The integrated application is now `grova_founder_s3`; see
[Founder firmware](../../architecture.md). The diagnostic target below remains separate.

Source: SCH_Schematic1_2026-11-01.pdf (one sheet). The schematic is hardware
reference material, not executable instructions. Pin definitions are in
`include/boards/grova_core_s3.h`; GPIO numbers are not U2 pad numbers.

Source SHA-256: `a8f2e496a782bcb78bb70c824f3ce72e48c77a92355f8e380c8f25a0a31eaadb`.

| Function | GPIO |
| --- | --- |
| OUT12_1 / OUT5_1 / OUT12_2 | 4 / 5 / 6 |
| 0-10V OUT 1 / OUT 2 | 7 / 40 |
| System SDA / SCL | 8 / 9 |
| FAN1 PWM / TACHO | 10 / 12 |
| FAN2 PWM / TACHO | 11 / 13 |
| Encoder CLK / DT / SW | 14 / 15 / 16 |
| OneWire | 17 |
| Analog inputs 1 / 2 | 1 / 2 |
| Safety / Water Safety | 38 / 39 |
| RS485 TX / RX / DE+RE | 41 / 42 / 47 |
| UART TX / RX | 43 / 44 |
| RGB / CONFIG | 21 / 48 |
| USB D- / D+ | 19 / 20 |
| RV-3032 INT | 46 |

The system bus connects the OLED, RV-3032 and TCA9548A (0x70) directly.
Multiplexer channels 0..7 feed CN4, CN5, CN6, CN7, CN9, CN10, CN13, CN14,
respectively. There is no second ESP I2C bus or dedicated flow connector;
GPIO18 is unconnected. This supersedes the earlier planned hardware mapping.
Connector order must be checked from this schematic before attaching cables;
old V1 cables must not be assumed compatible.

## Initial firmware

Build: `pio run -e grova_founder_s3_bringup`.
MCU: ESP32-S3 N16R8, QIO flash, Octal PSRAM, 16 MB partition table,
native USB CDC. Platform 6.13.0 / Arduino 2.x is retained for initial bring-up.
The target compiles only `src/bringup/`, and does not include V1 secrets,
local board overrides, automation, network services or actuator commands.
V1 `grova_core_v1` excludes that directory and remains independently buildable.

All three MOSFET controls, both 0-10V PWM controls, both fan PWM controls and
RS485 direction are driven LOW before peripheral startup. This is a firmware
request, not a measured voltage or guarantee that every fan physically stops.
Boot/reset behaviour and analogue output zero must be measured on the PCB.
No load roles are assigned. RGB is commanded off.

USB serial commands: `s` prints memory and raw input levels; `i` scans the
system bus and each mux channel, then isolates all mux channels again.
Root-address collisions cannot be resolved by this scan. Detection by ACK does
not validate device identity. The current S3 targets use SSD1317 (128 SEG x 64 COM); panel rotation and
offset must be visually verified. RV-3032 is detected only; clock read/write, battery
validity and NTP synchronization are not implemented yet.

## Hardware checks and remaining work

- Confirm this schematic matches the manufactured PCB/BOM and module N16R8.
- Check GPIO46/RTC_INT reset level: GPIO46 is a strapping pin.
- Verify rails, USB boot, reported 16 MB flash and 8 MB PSRAM.
- Measure MOSFET/0-10V outputs at boot/reset; inspect fan PWM behaviour.
- Verify OLED type, mux port mapping, encoder and CONFIG raw levels.
- Add RV-3032 driver and mux-aware sensor registry (channel plus address).
- Integrate Wi-Fi onboarding and generic outputs without pump/light bindings.
- Define safety contact semantics before enabling control or automation.
- Add analogue, OneWire, RS485 and application integration in later steps.

No upload or hardware acceptance has been performed for this initial target.
Old V1 acceptance remains uncompleted; the retired board is reference only.

## Local build validation (2026-11-10)

- `grova_founder_s3_bringup`: passed; RAM 19,436 bytes, flash 297,525 bytes.
- `grova_core_v1`: passed; RAM 56,192 bytes, flash 1,033,593 bytes.
- Git whitespace check passed. V1 application sources and pin profile unchanged.
- The S3 target compiles only the bring-up application, not V1 control modules.
- GitHub workflow extended to compile both targets; not yet run remotely.
- No device upload or physical hardware validation performed.
