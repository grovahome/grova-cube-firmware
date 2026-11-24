# Changelog

Notable firmware and repository changes. Development checkpoints are not production
releases; installed versions and physical acceptance are tracked in
[software validation](docs/software-validation.md).

## Unreleased

### Local controller interface

- Add a guided device setup and edit flow, a measurement history on the overview,
  and direct access to devices from the enclosure diagram.
- Offer only free ports when adding devices. Assign new 0–10 V devices to their
  signal output and retain previously saved paired assignments for compatibility.
- Leave all outputs paused when saving and review affected automations before
  resuming operation. Guide Wi-Fi setup through selection, testing and handover.

### OLED and rotary encoder

- Add SSD1312 and SH1107 profiles, including native 64×128 pixel mapping for SH1107.
  Select the controller, orientation and line offset under System → Display.
- Add an English OLED home screen and detail views for climate, outputs, fans
  and network. Turn to select, press to open or go back, and hold to return home.
- Count complete encoder detents and filter contact bounce. Fast turns are
  limited to one menu step per 140 ms and do not queue delayed steps.

- Show sensor source, sample age and lost-sensor status on the OLED.
- Add a saved idle choice after one minute of encoder inactivity: screen off,
  moving GROVA wordmark, or always on.
- Add timed output tests for both fans and free 5 V, 12 V and 0–10 V connections
  while automation is paused.

- Add daily controls on the OLED: confirmed pause and resume, configured
  device controls and 15-minute overrides for automated lights, fans and
  other non-timed outputs. The timed-device screen offers status and stop.
- Preview the next scheduled action on the home screen and in the web
  overview. Display brightness is saved in Settings and port tests are under
  Service.

### Branch organization

- Rename current development/default branch to `main` and frozen V1 reference to
  `legacy/core-v1`, preserving their commits and all release/freeze tags.
- Refresh local tracking references and clarify current versus historical profiles.

### Engineering workflow

- Run all eight mocked browser acceptance suites in GitHub Actions.
- Standardize LF text line endings and add contributor instructions.

### Repository and documentation — 2026-11-16

- Organize current application and tests under `controller/`; consolidate embedded
  tests under `tests/embedded/` and update build/include/CI references.
- Rename documentation and tools by purpose, preserving build and persisted identifiers.
- Refresh the README around GROVA's purpose, capabilities and hardware, with current
  prototype photographs, website button and a documentation index.

## S3 development checkpoint — 2026-11-16

Recorded installed version: `founder-dev.20261116.1`, source `4c04eaf`.

- Bridge short sensor reading failures using the last valid value of the same source
  without refreshing its timestamp; invalidate after 10 seconds and recover automatically.
- Simplify recent event rows to time and text; keep diagnosis links with current warnings.

## S3 development checkpoints — 2026-11-13

- Add per-device manual, scheduled and sensor-driven control, explicit sensor source
  binding, limits, hysteresis, delays, fallback and flexible fan roles.
- Add daily windows, intervals and bounded watering with persistent execution journals
  and restart protection; retain saved pause and configuration across power loss.
- Add rule diagnostics and central warnings.
- Add RAM event logging/export and measurement history, plus validated equipment/rule
  backup and paused restore without network credentials or execution journals.

See [board bring-up](docs/board-bringup.md) for earlier S3 engineering checkpoints
and outstanding physical acceptance. Entries below describe the legacy V1 line.

## v1.0.1

- Exported the standalone cube firmware into the clean `grova-cube-firmware` repository.
- Documented prototype hardware, pinout, safety notes, configuration, API examples, and roadmap.
- Kept MQTT as an optional interface for later local server, app, or cloud integration.

## v1.0.0

- First verified standalone firmware baseline.
- Added DHT22 temperature/humidity sensing.
- Added OLED display and rotary encoder UI.
- Added grow modes: germination, growth, harvest.
- Added automatic/manual light control.
- Added automatic/manual fan control.
- Added pump schedule, pump test, manual stop, and pump safety limits.
- Added persistent climate targets, warning limits, schedules, pump duration, and fan curve.
- Added OTA updates.
- Added local ESP web UI and versioned HTTP status/control/config APIs.
- Added optional MQTT telemetry, commands, and ACKs.
