# Founder software acceptance — 2026-11-12

## Installed checkpoint — 2026-11-12

Daily plans, configurable pulse recovery and version-2 execution journal are
installed from `6dde7cb` on the new Founder board at `192.168.1.115`.
Post-flash HTTP: healthy/settings OK, Wi-Fi/RTC/NTP working, Rest on, no device
assignments and all setpoints zero. Flash hash verified; no physical output
acceptance. See [first start](founder-first-start.md) for exact SHA and limits.
Dated "prepared/not installed" sections below describe earlier checkpoints.


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


## Daily plans — prepared after hardware disconnect

See [daily plan contract](founder-daily-plans.md). New code is NOT installed.
The user requested automatic continuation after power loss for scheduled devices;
boot now temporarily inhibits in RAM and restores only previously enabled plans
once time/storage/drivers/interlocks permit. Explicit Pause persists.

Added production-host, fresh-process persistence and browser acceptance tests
for multiple light windows, validated pulse events, durable high-water marks,
manual-stop persistence, clock loss, write failure and retained UI drafts.
This supersedes the old software-only assertion that no automation is supported;
manual requests still do not resume. Native PWM fan policy UI remains separate.
No physical outputs have been tested or switched for these offline checks.


### Configurable watering recovery and durable planned-run evidence

Pulse plans can skip or catch up the latest missed occurrence within 5/15/30/60
minutes. The old no-catch-up-only checkpoint is superseded. A version-2 journal
records the last planned event's actual software command phases; a reserved or
started event found on reboot is explicitly uncertain and is never rerun.
Completion is recorded only after software cutoff observation and a successful
NVS write; water delivery remains unmeasured. Creation baselines exclude events
from before the plan existed. Extra process-restart/fault-injection and browser
tests cover these contracts. Installation and exact binary/commit are recorded
in the subsequent first-start entry and internal software-validation record.


## OTA acceptance — 2026-11-13

Installed source `8f90329`, version `founder-dev.20261113.1`, target
`grova_founder_s3`, via ArduinoOTA/espota to `192.168.1.115` (UDP 3232,
TCP callback). Upload reached 100% and succeeded; no USB flash fallback.
Application SHA-256: `fa59ce4c6db9cfb4def1844f3aebbb07aa05e13302ca464978cd46960126614e`.

Overview now shows plan details and the next regular event only for assigned
devices with enabled plans. Planned pulse history is hidden for manual devices;
disabled ports disappear. Firmware/build and live OTA readiness are in Settings.

After reboot the same board reported the new version, healthy, WLAN connected,
OTA ready and valid RV-3032 time used at boot. Equipment revision 2, empty device
list, disabled native fan ports and persistent Pause matched the pre-OTA snapshot;
all five output setpoints remained zero. Populated plan persistence was covered
by host/browser tests, not by writing test assignments to the live board.

Founder host/fault-injection tests, 25 lifecycle and 17 Wi-Fi scenarios, JavaScript
syntax, both browser suites/mobile layouts and S3 build passed. Read-only browser
check of the actual LAN page passed without JS errors; writes were blocked.
Physical loads, sensor acceptance, SSD1317/encoder and power-path/recovery remain
open. This OTA acceptance supersedes older not-installed statements above.


## Independent fan operation — 2026-11-13

Every assigned PWM or 0–10 V fan independently selects `manual` or `automatic`;
there are no fixed intake/exhaust/circulation roles. A switched 5/12 V fan offers
the same mode choice but only on/off windows, never a fake speed percentage.

Manual operation offers a visible 0–100% slider on speed-capable ports or an
optional daily plan with up to eight windows and individual 1–100% demand.
Outside windows the fan is off; windows can cross midnight. One fan can provide
periodic circulation while another is reserved for future climate regulation.
Mode and plan are saved in the existing revisioned equipment configuration.
Old assignments without these fields migrate to manual without an active plan.
A disabled native port clears its plan when saved from the UI.

Automatic mode is a stored selection for future sensor rules. It currently
commands OFF and shows "not configured"; no inherited climate/interval policy
is silently enabled. The old policy editor and Founder policy commands are
retired. Future rules must use the same per-device demand/coordinator boundary.
The PWM driver still owns ramps, limits and latched tacho protection; periodic
demand updates do not acknowledge/reset faults and do not write NVS.

Native PWM daily plans join the existing explicit operation intent and valid-time
restart gates. Pause persists, loss of valid time stops scheduled demand, and
manual one-off demands are volatile and never replayed after a cold restart.
Direct manual commands cannot override an active plan or pending automation;
a stop request for such a fan persistently pauses automatic operation.

Host integration covers percent windows, midnight, fresh-process restart,
clock loss, Pause, invalid/overlapping modes/plans and failed configuration writes.
`check_fan_ui.cjs` covers both native connectors, independent roles, save/reload,
sliders, pending automatic mode, 0–10 V, switch limitations, removal and mobile
layout. Browser tests use fixtures; no physical fan or output was activated.


## Simple automation supersedes pending fan automation

See [simple automation](founder-simple-automation.md) for the current device-based
manual/time/sensor contract, exact-source rules, target and optional warning
limits, priorities, fallback, fan roles, intervals and bounded sensor watering.
Earlier statements that Founder sensor automation is not implemented are historical.
Installation is recorded separately after OTA verification.


Rule diagnostics / central warnings are covered by the additive evaluator/API tests
and tests/founder/check_diagnostics_ui.cjs. See founder-simple-automation.md.
No new output mode or authorization workflow; installed source recorded separately.


RAM event history: see founder-event-log.md. Pure ring tests, no-NVS/output-side-effect
API checks and check_events_ui.cjs cover bounded history and JSON download.
Existing five browser workflows remain unchanged. Installation is recorded separately.


RAM plot and equipment/rule backup: see founder-history-backup.md. Additional pure
history, production dry-validation/paused-restore/fresh-process tests and browser
check_history_ui.cjs / check_backup_ui.cjs cover these paths. No per-sample flash
writes; real board restoration is not part of read-only acceptance.
