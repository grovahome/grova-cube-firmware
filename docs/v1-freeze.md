# PCB v1 software freeze

Date: 2026-11-10
Branch: `grova-core-v1`
Annotated freeze tag: `grova-core-v1-final`

This tag preserves the existing ESP32 PCB v1 firmware before development for
new ESP32-S3 Founder hardware. New board work belongs on a separate branch.
No further feature development is planned for the old board.

## Scope

- Firmware code is unchanged from installed Cube 002 release `e58eb6d`.
- Previous HEAD `ee16f91` and this freeze only update documentation.
- Existing local documentation corrections distinguishing PCB v1 from Founder
  hardware are included. No local credentials or per-cube overrides are committed.
- Cube 001 / legacy DHT remains frozen and receives no OTA from this line.
- No OTA, dashboard deployment or Git push was performed for this freeze.
- Historical `v1.1.0` remains unchanged; this is a software baseline tag,
  not a declaration of completed hardware acceptance.

## Validation

`pio run -e grova_core_v1` passed on 2026-11-10:
RAM 56,192 bytes; flash 1,033,593 bytes. Git whitespace checks passed.
Host scenarios were not rerun for this documentation-only freeze because a
usable host C++ compiler was unavailable in the current environment.
The previously recorded validation for `e58eb6d` includes 25 lifecycle scenarios,
16 Wi-Fi scenarios, HTTP-route checks and embedded JavaScript checks.

Physical acceptance remains open: phone/OLED Wi-Fi onboarding (Cube 002 last
reported no OLED), automatic run completion with persistent Rest, offline reboot
and restart behaviour, independent pump cutoff timing, and reconnect/command
failure behaviour. Automated checks do not replace these measurements.

The installed Cube 002 release remains `e58eb6d`; this tag does not install a
new binary. See the internal GROVA `docs/software-validation.md` for deployment
evidence and the authoritative installed-version record.
