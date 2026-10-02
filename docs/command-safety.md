# Command freshness and pause/resume persistence

Development status: 2026-10-02; not yet deployed. These changes are versioned on
`grova-core-v1` as `Reject stale commands and persist pause transitions`.
Last deployed firmware is `d8f3243` (Cube 002 OTA 2026-09-22). The historical v1.1.0
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
open; no hardware validation or OTA was performed for this change.
