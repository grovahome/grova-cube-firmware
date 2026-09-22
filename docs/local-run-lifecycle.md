# Local run completion and Rest Mode

Scope: Cube 002, PCB v1, `grova_core_v1`. Legacy Cube 001 stays frozen.

## Behaviour

- The Cube owns completion: at the end of the final preset phase it records
  `completed`, clears active/paused, stops the pump, turns off light and fans,
  and enters persistent Rest Mode. No pump event is started at that boundary.
- Manual Stop Run uses the same shutdown path with status `stopped`.
- The old standalone pump schedule remains disabled after either transition.
- Presets and settings are retained. The last run ID, preset name and end time
  remain available in telemetry. A new explicit Start Run leaves Rest Mode.
- A repeated start revision cannot restart a finished run. Stop commands may
  include a run ID; a mismatching ID is rejected.
- Completion works without MQTT or the dashboard while the Cube has valid time.
  After a reboot without valid time an active run waits for time; an already
  persisted terminal run remains in Rest Mode even without valid time.
- Pause excludes paused time from run duration. Rest alone does not pause the
  duration of an active run.

## Persistence and telemetry

Run state is saved as one versioned Preferences blob. Existing per-key records
are read when no blob exists. The terminal record is written before the separate
Rest setting, so reboot between those writes still restores Rest Mode.
Outputs are parked before storage writes; failed terminal writes retry once per
second. No software can guarantee persistence if power is lost before any write
succeeds. An invalid stored blob fails safe into stopped/Rest Mode.

`local_run.completion_rest` advertises this lifecycle. `terminal_persisted`
reports successful run and Rest persistence; `ended_at_s` records the end time.
Status remains `completed` or `stopped` while the separate Rest flag is enabled.

The dashboard archives a matching run only on confirmed terminal telemetry,
including Rest Mode and successful persistence. Wall-clock expiry alone displays
`awaiting_completion`. Stop Run requires a successful device acknowledgement;
legacy firmware additionally requires an acknowledged Rest command.

## Validation

Run production lifecycle code against simulated clock, NVS and outputs:

```sh
python tests/host/run_tests.py
```

Requires a C++17 compiler (`--compiler PATH`; add `--zig` for Zig).
The 13 scenarios cover the final boundary, manual stop, reboot, pause/resume,
Rest expiry, future starts, missing time, explicit restart, failed writes,
power loss between run/Rest writes and legacy storage migration.
CI runs these tests before the regular firmware build.

Internal dashboard tests: `python -B -m unittest discover -s apps/grova-dashboard/tests -v`.
The 14 tests cover telemetry reconciliation, persistence, ACK failures and races.

The host tests and application build passed during implementation. OTA was
installed on Cube 002 on 2026-09-22. After reboot, its status reported
`completion_rest: true`, healthy/settings OK, Rest enabled, pump/light off and
both fans at 0%. Cube 001 was not updated. The dashboard changes are committed
only in the local internal repository; server deployment is separate.

A physical end-to-end completion test is still pending. On Cube 002, verify a short test run
ends with pump/light/fans off, remains in Rest after reboot and only starts again
on explicit Start Run. Never deploy this line to Cube 001.
