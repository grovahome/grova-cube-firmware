# Command freshness and pause/resume persistence

Deployment status: 2026-10-02. Firmware `2bafac6` was pushed on `grova-core-v1`
and OTA-installed on Cube 002. Dashboard commit `1586d9e` was deployed to the
local server after all 23 backend tests passed there. The historical v1.1.0
tag is unchanged. Only PCB v1 / Cube 002 receives work on this line.

## Command contract

Commands may carry `expires_at_s` (positive epoch seconds). When present, the
firmware requires synchronized time and rejects commands at or beyond expiry
before changing outputs or settings. Invalid/zero expiry is rejected. Commands
without this field remain compatible with local and older clients.

The updated dashboard supplies expiry for every command and cancels unsent
commands when its ACK wait ends. Disconnected or full queues reject new commands.
MQTT publication is non-retained. Firmware rejects repeat IDs among the last 16
MQTT commands during a boot; this is not persistent exactly-once delivery.
No deadline protection is claimed for raw commands without an expiry or the
frozen legacy firmware. A timeout cannot retract a command already sent.

Stamped commands fail closed without usable time, including stop requests.
The local fallback API can still issue an unstamped stop. Run-ID checks apply
to Stop, Pause and Resume when a run ID is supplied.

## Storage failure

Pause/resume requires usable time. A repeated request for the current state is
idempotent. Resume saves the adjusted start time before allowing execution;
failed storage keeps the prior paused state. A failed pause write stops and
parks the run in Rest Mode, retries terminal persistence, and returns failure.
Power loss before any successful write remains a physical validation limitation.

## Verification

The host suite adds expiry boundary/no-clock, duplicate ID, pause-write failure,
resume-write failure and missing-clock checks to the existing lifecycle scenarios.
Run `python tests/host/run_tests.py`, then `pio run -e grova_core_v1`.
On 2026-10-02 all 17 host scenarios and the application build passed (55,760
bytes RAM, 1,014,949 bytes flash). The internal dashboard suite passed 23 tests.
Physical completion/reboot/offline/new-start and pump-cutoff timing tests remain
open. Post-OTA status confirmed the Oct 2 build, healthy/settings OK, valid time,
Rest enabled, and pump/light/both fans off. The dashboard restarted healthy with
MQTT connected and both cubes online. These status checks do not replace physical
acceptance tests. Cube 001 firmware was not updated.
