# Founder ESP32-S3 application

Status: 2026-11-10. Development branch `codex/founder-pcb-bringup`, based on
corrected V1 baseline `5725652`. The V1 branch and freeze tag are unchanged.
No S3 upload, physical acceptance or dashboard deployment has been performed.

## Build and separation

`pio run -e grova_founder_s3` builds the integrated application and is this
branch's default. `grova_founder_s3_bringup` remains the minimal serial diagnostic
image. `grova_core_v1` remains a separate build for reference, not a target for
the new PCB. Platform 6.13.0 / Arduino 2.x supports the shared LEDC fan driver;
Arduino 3 migration is not required for initial S3 operation.

Both S3 targets use 16 MB QIO flash and 8 MB Octal PSRAM. Check actual reported
memory on the PCB; a successful compile does not verify the fitted module.
The integrated target deliberately excludes old `main.cpp`, grow phases,
light/pump drivers, local runs, legacy web UI and legacy MQTT device discovery.
V1 local `secrets.h` and `board_config.h` are not read by the S3 application.
MQTT settings are entered through the local webapp instead of compiled secrets.

## Layers

- Board: `include/boards/grova_founder_s3.h`, derived from the versioned schematic.
- Bus: `founder/i2c_bus`, a mutex-owned scope selects one TCA9548A channel and
  restores root isolation afterwards. No consumer selects channels on its own.
- Devices: `founder/devices`, one identity per root/channel/address. Supported
  drivers initialize, measure, retry and publish real sample timestamps.
- Signals: shared `modules/signal_registry`, values, source, unit, age, validity.
- Functions: existing fan device/arbiter/policy layers consume abstract signals;
  generic MOSFET and analogue output service has no pump or light roles.
- Application: neutral targets, Rest, persistence and validated command handling.
- Presentation/transports: OLED/encoder, local phone webapp and optional MQTT.

The current source selection is deterministic: SHT4x, AHT20, SCD41, then BME/BMP;
within each family root precedes mux channels 0..7. Temperature and humidity
may fall back independently. All detected instances are visible in the device
list. User-defined signal routing and the future automation editor remain work
for a later step; no microgreens phases are imposed on the new application.

## Initial setup and controls

Without saved valid credentials, the existing protected setup AP opens. OLED
shows SSID, generated setup password and AP IP. Connect the phone, open that IP
(or `/wifi`), enter the OLED setup key, scan networks and test credentials.
Only a successful connection with DHCP and successful storage replaces the old
network. The existing rollback, throttle and AP-interface restrictions remain.
This is Wi-Fi provisioning, not the final product onboarding wizard.

After joining the LAN, open the board IP for the neutral webapp: signals,
devices, both fans, three switch outputs, two 0-10V setpoints, RTC, targets and
MQTT configuration. CONFIG held for five seconds enters Rest and opens setup
without deleting the last validated WLAN. Encoder rotation/click navigates four
OLED information pages; it does not activate outputs. OLED assumes SSD1306
128x64, probes 0x3C/0x3D on root and retries initialization if missing.

The application always boots in Rest and clears all five output commands.
Leaving Rest requires an explicit command and successful Rest persistence.
Rest/setup/OTA park generic outputs at zero. OTA also latches inhibition until
reboot and stops both fans. Entering/leaving Rest never replays generic output
values. Fan settings may persist, but Rest prevents applying them until release.
Fan hardware/device stall, boost, ramp and arbitration layers are reused.

Outputs use neutral IDs `OUT12_1`, `OUT5_1`, `OUT12_2`, `0_10V_OUT_1` and
`0_10V_OUT_2`. Switches accept 0/100 percent or a Boolean state. Analogue outputs
accept 0..100 percent (0..10 V nominal) using separate 5 kHz / 12-bit LEDC
channels 2/3; fan channels remain 0/1 at 25 kHz / 8-bit. The voltage shown is a
setpoint, not a measurement. These are general manual channels: no pump role,
automatic pump schedule or independent timed pump cutoff is assigned to them.

## I2C devices

Native candidates currently ported from the previous line: SHT4x at 0x44/0x45
(serial/measurement CRC checked), AHT20 at 0x38, BME280/BMP280 at 0x76/0x77,
VEML7700 at 0x10, SCD41 at 0x62 and LTR390 UV at 0x53. A device ACK alone is not
an active measurement; the driver must initialize and obtain valid data.
Exact identity validation depends on the chip's identification protocol.
LTR390 UV uses the previous fixed conversion divisor, not a new calibration.

Discovery/reinitialization is retried about every ten seconds per location;
sampling is staggered and stale signals expire after twelve seconds. Disconnects,
CRC failures and read failures invalidate data and permit later reinitialization.
Same-address devices on different mux ports are separate instances. A root-bus
device with the same address as a downstream device cannot be isolated by this
hardware; downstream candidates at that address are excluded while root is present.
This does not certify live electrical hot-plugging or bus-short recovery.

## RV-3032 time

Dedicated driver uses UTC calendar registers, validates BCD/calendar/leap dates,
and refuses PORF, VLF and STOP states. RTC time seeds the system when valid.
Only a real SNTP callback authorizes writing system time back to the RTC;
Wi-Fi connectivity alone does not prove the clock was synchronized.
Calendar writes use STOP, full write, restart, flag clearance and readback.
A failed calendar write leaves STOP asserted so torn time is not used at boot.

The RAM PMU mirror selects level-switching backup with charging disabled;
automatic EEPROM refresh is disabled in RAM so that setting remains active.
No EEPROM programming or battery charging is performed. Configure on every boot,
verify the battery and measure retention after real power loss.
RTC INT is not used for wake or interrupts by this application.
Register reference: [Micro Crystal application manual, rev 1.3](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3032-C7_App-Manual.pdf).

## API and MQTT

GET `/api/v1/status` (alias `/api/status`) returns neutral capabilities,
per-instance device data, six abstract signals, fan status and generic outputs.
POST `/api/v1/control` requires JSON and is disabled on the setup interface.
Commands: `set_rest_mode`, `set_output`, `set_fan_manual`, `set_fan_auto`,
`set_fan_policy`, `set_targets`, `set_rtc_config`, `set_mqtt_config` (HTTP only).
Old pump/light/grow commands are rejected, not redirected to a guessed channel.

The optional broker is configured locally (host, port, user, password). Empty
host disables MQTT. Broker password and WLAN setup key are absent from status.
The device ID is MAC-derived and does not impersonate Cube 001 or 002.
Topics remain `grova/v1/cubes/<id>/{telemetry,command,ack,availability}`.
Remote commands require `cmd_id` (max 47 chars), nonzero `expires_at_s` and a
usable clock; expired or repeated IDs are rejected. Deduplication is bounded to
16 RAM entries and does not promise durable exactly-once execution.
The new MQTT packet does not claim compatibility with old automation controls;
server hardware-profile/UI integration and Home Assistant discovery are pending.
Broker transport and local control retain the LAN-only trust model of the
prototype; no hosted authentication/TLS deployment is introduced here.

## First-board acceptance

1. Confirm manufactured schematic/BOM, rails, USB enumeration, flash and PSRAM.
2. Check GPIO46/RTC_INT: the schematic pulls it up through R58. Espressif requires
   GPIO46 LOW/floating when GPIO0 is LOW for joint download boot. Review the
   manufactured circuit with the PCB developer before relying on BOOT for first
   programming. Firmware cannot fix a reset-time strap before it executes.
   [Espressif boot mode reference](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html).
3. Measure switch/0-10V/fan states during power-up, reset, Rest and OTA.
4. Run phone/OLED onboarding with good/bad credentials and power interruptions.
5. Attach AHT20/SHT4x on each mux port, test removal/reconnection and two equal
   addresses on different ports. Verify readings against a reference.
6. Confirm RTC UTC/NTP operation, battery switchover and power-loss retention.
7. Check encoder/CONFIG, both fan tacho channels and output stages under load.

These remain physical OPEN checks. Automated tests do not constitute acceptance.
Safety/Water Safety contact semantics, OneWire, ADC and RS485 application support
remain unassigned/unimplemented; their pins are documented, not falsely advertised
as active interlocks. No new automation engine or old pump role is enabled.

## Validation of this development step

- Integrated S3 application build passed: RAM 61,980 bytes; flash 944,585 bytes.
- Minimal S3 diagnostic and V1 reference builds passed.
- Previous 25 lifecycle scenarios, 16 Wi-Fi scenarios and production Wi-Fi
  HTTP authorization/throttle/interface checks passed on the final shared code.
- New host harnesses exercise production bus/RTC/output services, production
  command/status code and production device management. Cases include invalid
  BCD/calendar/PORF/VLF/STOP, torn calendar writes, mux selection failure,
  same-address sensors on two channels, sensor removal/CRC failure/recovery,
  setup/Rest/OTA output inhibition, wrong JSON types, expiry, duplicates,
  target-persistence failure and real sample timestamp retention.
- Embedded web JavaScript syntax passed. Headless Edge with simulated API data
  verified Rest restrictions, switch and analogue command payloads, no script
  errors and a mobile layout without horizontal overflow. This is UI simulation,
  not an ESP HTTP, radio or electrical test.
- CI now includes all three builds, Founder host checks and JS syntax; these
  workflow additions have not yet run remotely because this branch is local.
- No new firmware was uploaded; the internal GROVA working copy was untouched.
