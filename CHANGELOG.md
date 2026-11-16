# Changelog

Notable firmware and repository changes. Development checkpoints are not production
releases; installed versions and physical acceptance are tracked in
[software validation](docs/software-validation.md).

## Unreleased

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
