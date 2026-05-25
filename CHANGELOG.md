# Changelog

All notable firmware changes should be documented here.

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
