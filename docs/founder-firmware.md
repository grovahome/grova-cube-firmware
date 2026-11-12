# Founder ESP32-S3 application

Status: 2026-11-12. Development branch `codex/founder-pcb-bringup`, based on
corrected V1 baseline `5725652`. The V1 branch and freeze tag are unchanged.
The previous checkpoint was installed and Wi-Fi/LED/RTC checks are recorded in
[First start](founder-first-start.md). The new [daily plans](founder-daily-plans.md)
are prepared in source only: controller disconnected, no new flash or physical
output acceptance. The internal dashboard deployment is unchanged.

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
OLED information pages; it does not activate outputs. OLED uses the user-confirmed SSD1317 and a 128 SEG x 64 COM framebuffer,
probes 0x3C/0x3D on root and retries after missing/failed transfers. Rotation
0..3 and COM offset 0..95 are persistent settings. The provisional default
(rotation 0, offset 80) follows a manufacturer 64-MUX reference panel; exact
module mapping remains to be verified on the supplied display.

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
`set_fan_policy`, `set_targets`, `set_display_config`, `set_rtc_config`,
`set_mqtt_config` (HTTP only).
Old pump/light/grow commands are rejected, not redirected to a guessed channel.

The optional broker is configured locally (host, port, user, password). Empty
host disables MQTT. Broker password and WLAN setup key are absent from status.
The device ID, setup SSID and OTA hostname use all six factory MAC bytes.
The older low-24-bit suffix could select only shared manufacturer bytes; the
S3 path now avoids that collision and does not impersonate Cube 001 or 002.
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

## First-board preparation 2026-11-12

The user confirmed controller SSD1317 and display size 64x128. A dedicated
controller driver replaces the SSD1306 assumption in both S3 targets; the V1
OLED driver is unchanged. The graphics canvas is converted to SSD1317 page
bytes, uses bounded I2C packets and checks ACKs on initialization and updates.
Offset/rotation are explicitly configurable because the controller alone does
not identify panel wiring. ACK means reachable/initialized, not optical proof.

References for controller/panel development:
- [Solomon Systech SSD1317 controller datasheet](https://www.displayfuture.com/Display/datasheet/controller/SSD1317.pdf)
- [Wisevision 64x128 SSD1317 panel reference, section 4.4](https://www.jx-wisevision.com/uploads/SPEC-X154-6428TSWXG01-H13-VER-A.pdf)
These are reference documents; the supplied module's vendor/part number has
not yet been established. The controller driver does not generate panel VPP;
the attached module must supply its own required OLED power circuitry.

USB console commands (newline terminated):
- `status`: full diagnostics without passwords, including OLED/driver readiness.
- `setup`: enter Rest, open recovery AP and print its existing setup credentials
  explicitly on the local physical USB console. Does not erase saved WLAN.
- `display 0 80`: save rotation/offset even before WLAN provisioning works.
  Rotations 1/3 provide portrait layout; choose the actual physical orientation.

A failed analogue PWM initialization now also fails reported output readiness
and overall health; positive generic output commands remain blocked. Hardware
readiness is not a measured output voltage. The firmware still does not claim
that every connected four-wire fan stops physically at a zero PWM request.

`python tools/package_founder.py` builds the integrated target and writes an
ignored `dist/founder-s3/` first-flash package with component addresses and
SHA-256 hashes. It does not communicate with or flash any board. The merged
image starts at 0x0 and overwrites the intervening NVS range; reserve it for the
first flash of a blank board. Use normal PlatformIO application upload/OTA for
subsequent configured-device updates. See [today's bring-up guide](founder-first-start.md).

Validation on 2026-11-12: all three PlatformIO builds passed (integrated S3:
62,052 bytes RAM / 954,709 bytes flash). Existing 25 lifecycle and 16 Wi-Fi
scenarios plus Wi-Fi HTTP checks passed. Founder host checks additionally
exercise actual SSD1317 packet transmission failures and canvas conversion,
full-MAC name collisions through the S3 Wi-Fi/OTA path, display parameter bounds
and failed PWM initialization. Browser simulation passed for missing OLED,
SSD1317 setting commands, output readiness blocking, Rest and mobile layout.
All physical checks, especially the exact OLED panel mapping and GPIO46 reset
behaviour, remain OPEN. No device was uploaded to by these checks.

## Flexible device assignment 2026-11-12

The previously neutral output channels now have an optional persistent device
catalog/assignment layer. Raw commissioning controls are restricted to local,
unassigned outputs; device commands use profile-specific protections. See
[device catalog and assignment](founder-equipment.md) for supported equipment,
5/12 V compatibility, 0-10 V control vs power, timed pump/valve cutoff, limits
and the remaining physical acceptance checks. Earlier no-role descriptions
above document the preceding migration stage. Automation stays deferred.

Validation for the assignment layer: Founder production driver/API tests passed,
including timer initialization failure, GPIO cutoff independent of the app loop,
assignment persistence/rollback, voltage/type/ownership checks and timed command
replay. The previous 25 lifecycle and 16 Wi-Fi scenarios plus production Wi-Fi
HTTP checks passed. Embedded JS syntax and a mobile Edge simulation using actual
production status JSON passed (selection/save payloads, retained drafts, assigned
output lockout, timed start/stop). All three PlatformIO targets passed; the new
S3 application uses about 63.3 kB RAM and 983 kB flash. These are local software
checks; no physical acceptance, upload, push or deployment was performed.

## Software acceptance before board arrival

The [Founder software acceptance record](founder-software-validation.md) covers
empty first start, all implemented device profiles, fresh-process restart with
saved state, corrupt/unavailable storage, editor revision conflicts and lost
control/status responses. The preview now starts empty; demo assignments are
explicitly separate. Profile limits and missing monitoring are visible in the
UI/catalog. These checks do not close any physical acceptance item.

## OLED welcome animation

The integrated Founder target draws a 2.2-second monochrome G/leaf mark, staged
GROVA wordmark, CORE subtitle and eased progress bar once per boot. It uses a
small code-defined bitmap/font and the existing SSD1317 canvas; no image files,
NVS completion flag, animation delay loop or output commands are added. The bar
is decorative and does not certify hardware readiness. It fills shortly before
the welcome finishes, then normal status or Wi-Fi setup credentials take over.
The display is primed before networking starts; subsequent frames are scheduled
from the regular loop at a nominal 100 ms interval. Actual I2C timing remains a
hardware check. Both logical 128x64 and 64x128 orientations are supported.

Encoder navigation skips the welcome, changing panel configuration cancels it,
and reconnecting an OLED later does not replay it. An OLED first found more
than eight seconds after display initialization goes directly to normal status.
The existing SSD1317 initialization wait remains unchanged.

Host tests cover frame bounds/progress/completion in portrait and landscape,
clock wrap, cancellation and late detection. A separate harness compiles the
production display service with deterministic canvas/I2C adapters and verifies
frame cadence, Wi-Fi handoff, user skip, disconnect/recovery and rotation change.
`tests/founder/run_tests.py --boot-frames path.json` exports the actual renderer
frames for a browser/GIF preview; it does not read or control a connected board.
The welcome is not yet optically verified on the physical SSD1317 module.
