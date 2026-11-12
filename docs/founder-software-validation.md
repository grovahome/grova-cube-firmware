# Founder software acceptance — 2026-11-12

Scope: software-only acceptance before the new PCB arrives. Founder branch,
`grova_founder_s3`; no deployment, OTA, hardware actuation or changes to the
internal GROVA worktree. Installed versions are still documented separately in
the internal `docs/software-validation.md`. This page does not certify hardware.

## Live hardware checkpoint — 2026-11-12

The pre-arrival software scope above is historical. The Founder application has
now been flashed to the new ESP32-S3 N16R8 board; full observations are in
[First start](founder-first-start.md). Confirmed: real phone Wi-Fi setup and
persistence, restart reconnect, RTC valid boot time after a short battery-backed
power interruption, and visually correct RGB sequence/status. A reproducible
unsigned timestamp underflow in queued Wi-Fi trials was fixed and regression
covered with nonzero radio startup time.

Setup AP uses an eight-character password; no duplicate entry on its AP page.
It closes five seconds after successful save. The setup links now point to the
actual home-LAN app address. SSD1306/SSD1317 profiles are explicitly selectable;
SSD1306 worked, the supplied SSD1317 modules did not ACK. Display/encoder are
currently deferred. No output tests: WAGO connectors are not yet soldered.
Sensors, output electrical limits/cutoffs, long-term RTC retention, power-path
rework and manual recovery remain open. Internal Cube 001/002 deployments are
unchanged.

## Verified behavior

| Area | Verification and result |
| --- | --- |
| Empty first start | Production boot safety routine forces GPIO/fan preinit off and Rest on. Fresh host process with empty NVS has no devices, no occupied output and no enabled PWM fan port. |
| Startup readiness | Missing app/equipment storage, empty corrupt configuration/event records and failed output initialization prevent release. Explicitly saving a valid configuration can repair the corrupt configuration case; corrupt event history remains blocked. |
| Device catalog | All seven profiles exercise their real manual start/set and stop paths. Pump/valve require timed commands; timer cutoff is tested without the app loop. Assigned raw-channel bypass is rejected. |
| Voltage / ownership | Fixed 5/12 V metadata is transmitted by the UI and validated independently by firmware; incompatible types and duplicate power/output assignments are rejected. |
| Persistence / rename | Two separate host processes share only an exported fake-NVS byte snapshot. Save and rename, start pump/light, terminate without graceful stop, then restart: names/assignments persist, outputs remain zero/locked, previous pulse event remains consumed. |
| Removal | Requires lock, removes the saved device from overview and rejects further commands for its removed ID. Reopening the UI retains the empty configuration. |
| Save errors | Failed NVS writes preserve old mapping. UI preserves the unsaved draft and old overview on rejected saves. |
| Concurrent editors | UI submits `expected_revision`; firmware rejects stale/non-integer supplied revisions before writing. UI blocks outdated drafts until the user reloads the current mapping. Legacy API requests omitting the field are still supported. |
| Lost response | No automatic POST retries. Unknown command execution is reported explicitly; a pending configuration is reconciled from subsequent status. A failed status read after a successful ACK never rebuilds the editor from stale state. |
| Browser offline / timeout | Requests abort after 5 seconds, overlapping status polls are suppressed, output state is marked unknown and controls are gated. A later successful poll restores operation without erasing the draft. This does not imply physical loads stop on browser disconnection. |
| Interface / layout | Fixed connector editor, read-only enclosure selection, future-interface placeholders, keyboard selection and mobile width remain covered by browser checks. |
| Preview | Starts with a verified empty-board fixture and a visible no-hardware banner. Example pump/light assignments load only through an explicit preview preset; no demo data is compiled into production device defaults. |

The host runs use the production app, equipment registry, output/timer and Rest
sources with deterministic adapters. Separate bus/RTC/sensor/WLAN tests remain
part of the Founder test runner. Hardware drivers, radio behavior, actual NVS
power-loss atomicity and cutoff timing still need validation on the board.

## Profile protection boundaries

All configured devices inherit exclusive output ownership, fixed output type /
voltage checks and Rest/setup/OTA inhibition. After boot, requests are not resumed.

| Profile | Additional implemented behavior | Not provided / physically unverified |
| --- | --- | --- |
| DC pump | Hardware-timer pulse cutoff, max 10 s configurable lower; 10 min boot lock; last 16 slot/event IDs reserved before energizing | No dry-run, flow, water-level, current or temperature monitoring; no total-volume/daily-budget limit. Actual cutoff must be measured. |
| Normally closed pulse valve | Timer pulse cutoff, max 10 s configurable lower; bounded durable event history | No position/flow/current/temperature feedback; actual fail-closed behavior depends on the real valve. |
| Switched light | Boolean switching | No current/temperature monitoring or physical light feedback. |
| 0-10 V light | Bounded 0..100% setpoint; optional exclusively owned power companion, power-on-before-setpoint and setpoint-zero-before-power-off | No voltage/current/temperature measurement. Dim-to-off depends on external driver. |
| Switched DC fan | Boolean switching | No tacho, current or temperature feedback. |
| 0-10 V fan | Bounded 0..100% setpoint | No tacho/current/temperature feedback. A zero signal may mean minimum speed on the real fan. |
| General switch | Boolean switching | No load-specific pump/thermal protection; must not be used as a bypass profile. |

Dedicated PWM/tacho fan connectors use the existing fan service and fault logic;
they are distinct from the generic switched/0-10 V profiles. No new fan driver
or feedback channel is introduced here. Output current ratings remain unknown.
The catalog exposes `missing_protections` and `physical_acceptance: OPEN`; the
selected profile's UI displays these limits instead of promising all protections.

## Reproduce

After building `grova_founder_s3` (for the real ArduinoJson dependency):

```sh
python tests/founder/run_tests.py --status-output /tmp/founder-example.json --blank-output /tmp/founder-blank.json
python tests/founder/check_web.py
node tests/founder/check_ui.cjs --blank /tmp/founder-blank.json --playwright /path/to/playwright --browser /path/to/browser
python tools/preview_founder.py --blank-status /tmp/founder-blank.json --example-status /tmp/founder-example.json --output /tmp/founder-preview.html
```

Paths above are command examples. On Windows, use actual temp paths and the
installed compiler/browser; host runner accepts `--compiler ... --zig`.
`check_ui.cjs` uses an existing Playwright installation and a headless browser;
when `--browser` is omitted it uses Playwright's installed Chromium. Add
`--preview /path/to/founder-preview.html` to also check the separated presets.
No npm dependency or browser is silently installed by this script.

Existing CI invokes the expanded C++ host runner and JS syntax check. The new
browser runner has been verified locally, not as a remotely executed CI job.
Hardware acceptance and the first-board checklist remain OPEN.

Final local results: all three PlatformIO environments passed, including frozen
V1 (no production behavior changes in that target). Existing 25 lifecycle and
16 Wi-Fi scenarios plus production Wi-Fi HTTP checks passed with the extended
test adapter. Founder driver/app, all seven profiles, fresh-process restart and
new browser acceptance checks passed. S3 RAM: 63,384 bytes; flash: 1,018,985 bytes.
No remote CI run or hardware installation was performed.
