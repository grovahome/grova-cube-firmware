# Founder daily device plans — 2026-11-12

Status: implemented and host/browser tested; NOT installed on the disconnected
controller. Electrical output acceptance is still open (WAGO connectors not yet
soldered). Installed hardware observations remain in founder-first-start.md.

## Simple local workflow

Choose a fixed connector in the controller view, activate it, choose the device,
set its daily times and press **Speichern**. Voltage follows the physical port.
Names are prefilled and can be changed. Selecting a new light offers an explicit
08:00–20:00 plan which can be edited or unchecked before saving. It is not
energized by selecting a connector or changing a draft.

Each generic assigned device has up to eight daily entries:

- Switched/0–10-V light, generic switch and DC/control fan: ON windows; outside
  the union the request is zero. Analogue windows also have a percentage.
- Pump and normally closed pulse valve: start time and bounded duration, never
  a continuous ON window. Existing per-profile/per-device runtime limits apply.
- The two native PWM/tacho fan ports retain their separate existing policy UI;
  this daily editor does not yet replace their control policy.

Times use the Core's configured central-European local timezone, including DST,
not the browser's timezone. Windows may cross midnight; end 00:00 means midnight
and a 00:00–00:00 entry is serialized as 0–1440 for all day. Overlapping windows,
duplicate pulse times, invalid types, empty enabled plans and invalid durations
are rejected. The editor preserves unsaved drafts across status polling.

## Save and restart behavior

Configuration and plans share one validated, revisioned NVS configuration blob.
Existing configurations without plans remain manual. Size is checked against
the same 4096-byte bound on save and reload. A successful write precedes mapping
replacement. While editing an active system, Save first pauses all operation,
saves configuration, then enables the explicitly selected plans. Lost ACKs are
reconciled from revision/status; POST requests are not repeated automatically.
A failure or power loss before the final enable acknowledgement can leave the
operation paused; the UI must confirm its actual state.

Boot always preinitializes outputs OFF. The desired pause/release setting is
retained instead of being overwritten at every boot. A checksummed application
intent record AND an explicitly saved Rest release must agree; missing or corrupt
intent/Rest records cannot implicitly arm a plan. Only a previously released
system with enabled plans, valid RTC/NTP time, ready storage/output drivers and
no provisioning/OTA interlock is automatically released. Saved manual ON demands
are never replayed. An explicit Pause/stop_all is durable, so a reboot does not
undo it. No valid time means planned outputs stay OFF. A router outage alone
does not prevent plans when saved WLAN credentials and valid RTC time exist.

## Pump execution and interruption

A dedicated checksummed NVS ledger holds a monotonically increasing date/minute
high-water mark per device slot. It is written BEFORE a planned pulse can drive
a GPIO. An interrupted or uncertain start stays consumed; neither reboot,
repeated loops, saved-plan edits nor a backward clock step repeats that event.
A ledger write/read failure blocks operation and turns outputs off.

Missed-event handling is selectable per pulse device: skip (default), or catch
up once within 5/15/30/60 minutes. Only the latest due occurrence is considered;
there is never a queue of accumulated doses. Previously consumed/started,
skipped or uncertain events are not repeated. This also applies when resuming
from Pause, so the UI explicitly exposes the catch-up choice. Occurrences before
plan creation are excluded; when time was unknown at setup a baseline is saved
once valid time becomes available. This does not guarantee a crop receives a
missed dose after a long outage or a short catch-up window.

The version-2 checksummed journal separately stores reservation, command start,
normal completion, explicit interruption, uncertain restart, rejected start and
skipped event. Name/output, scheduled date/minute, requested duration and command
timestamps are saved. Completion requires the software output to be off after
its monotonic deadline and a successful final journal write. A reboot with a
reserved/started record becomes INTERRUPTED_UNKNOWN, never COMPLETED, and has
unknown elapsed time. GPIO commands are not proof of actual water delivery.
A failed reservation never starts the output; failed start/completion writes
force OFF and a blocked state. The legacy high-water ledger migrates without
inventing successful runs (LEGACY_UNKNOWN). Only the last planned event per
logical device slot is retained, not an unlimited log or manual-run history.

Pump boot lock remains 10 minutes; max runtime is 10 seconds or a lower saved
limit. The existing timer ISR cutoff bypasses the normal app loop. This is not
an independent external hardware cutoff and cannot guarantee safety under every
MCU or MOSFET failure. Flow/dry-run/water-level/current/thermal feedback is still
absent. Do not select a generic switch to bypass timed pump protections.

## Validation

- Pure schedule tests: exclusive end boundaries, multiple windows, overlap,
  midnight/all-day windows, duplicate events and duration limits.
- Production app/output tests: time unavailable, automatic light start/stop,
  durable global pause, invalid edits, missing/corrupt ledger and write failure.
- Separate host processes with shared NVS bytes: restart during a reserved pump
  pulse never replays it; next scheduled event runs; paused system stays paused.
- Browser workflow: fixed 12-V port → light → two periods → save → reload,
  overlap rejection, active edit with pause/save, deletion, pulse-only pump UI,
  no hardware requests and mobile layout.

Physical scheduler switching, real cutoff timing, DST transitions on the board,
long outages and measured flash-power-loss atomicity remain unaccepted.

The additional restart tests cover missing/corrupt operation intent and a failed
first Rest-release write. Neither may arm outputs after reboot. The shared Rest
store now writes an explicitly requested first value even if it equals its
default, so an acknowledged release has a real persistent record. Legacy host
lifecycle and WLAN tests still pass.


Additional validation covers: durable completion across a fresh process,
simulated power loss immediately after reservation and before GPIO, failed
start/completion journal writes, explicit interruption without replay, migration
of version-1 history without claiming completion, year/leap-day rollover and
persisted catch-up choice / uncertain-run presentation in the browser.
