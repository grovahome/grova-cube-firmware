# Pump Safety

Pump safety applies to automatic pump runs and local preset pump events. Manual pump tests are available for setup and maintenance, but Rest Mode and the runtime limit still apply.

## Automatic Pump Event Lock

```text
Scheduled event lock: once per date/minute event
Startup lock after reboot: 10 minutes
Maximum configured pump runtime: 10 seconds (cutoff tolerance below)
Automatic pump duration: configurable from 1 to 10 seconds
No hard automatic run count per day
No hard minimum interval between distinct scheduled events
```

Automatic pump starts are blocked when:

- Rest Mode is active
- time is not synchronized
- grow mode is Harvest
- the startup lock is still active
- the same scheduled date/minute event already started
- the requested runtime is outside `1..10` seconds
- the runtime guard is unavailable or inhibited

## Pump Tests

Pump tests:

- do not count as scheduled pump events
- are not blocked by the startup lock
- are blocked by Rest Mode
- are limited by the same runtime guard as automatic runs

## Grow Mode Interaction

```text
GERMINATION: automatic pump events remain active
GROWTH:      automatic pump events remain active
HARVEST:     automatic pump events are blocked
```

## PCB v1 Runtime Guard

Both scheduled watering and manual tests use `pump_startTimed()`. There is no
unbounded `pump_on()` entry point.

### Behavior

- The output is LOW and the guard initialized before Wi-Fi startup.
- Each start arms an absolute monotonic deadline before energizing the output.
  Zero durations, durations above 10 seconds, and starts while running are
  rejected. Repeated requests cannot extend an active run.
- ESP32 timer group 1 / timer 0 is reserved for pump safety. Its 1 ms periodic
  IRAM interrupt checks the deadline and clears the GPIO directly, independently
  of the application loop. Delayed interrupts do not accumulate missed ticks.
- The cutoff path uses no logging, allocation, flash code, or task-dispatched
  timer callback. Its timing is the deadline plus up to 1 ms and interrupt latency.
  CPU failure, disabled interrupts, or a failed MOSFET still require a separate
  electrical safety mechanism; this is protection against application blocking.
- Initialization failure prevents all starts until reboot. Rejected HTTP/MQTT
  pump tests report failure. Scheduler telemetry reads the driver's running state.
- OTA start stops and inhibits the pump until reboot, including after OTA errors.
- Existing Rest Mode, automatic startup lock, time/Harvest checks, event lock,
  and configured pump durations remain in the scheduler.

### Verification

Build application firmware: `pio run -e grova_core_v1`.

Compile bench tests without upload or execution:

```powershell
pio test -e grova_core_v1 -f test_pump_safety --without-uploading --without-testing
```

This command only compiles; its PASSED label is not a hardware-test result.
Rebuild with `pio run -e grova_core_v1` afterwards, because test compilation uses
the same output directory and its `firmware.bin` contains the bench test program.
CI compiles the tests first and builds the application last for this reason.

The tests compile the actual driver and inject timer initialization/start failures.
They cover invalid durations, duplicate starts, cutoff during a busy-wait,
stop/restart across an old deadline, the 10-second limit, and the OTA interlock.

Run hardware tests only on a dedicated ESP32 bench with the pump/load disconnected
and PCB v1 pins verified. Tests intentionally energize the GPIO and replace the
application firmware. Do not upload to Cube 001 or an operating grow cube.

Before application deployment to Cube 002, measure 1, 5, and 10 second output
pulses with a logic analyzer. Acceptance target with normal interrupt service:
no pulse more than 2 ms over its configured duration. Repeat with a broker that
accepts TCP but does not answer MQTT, slow HTTP requests, and NVS writes. Verify
immediate LOW at OTA start and continued inhibition after an OTA error.
Compilation alone does not verify physical timing or OTA behavior.

## Production Notes

Before long-term unattended use, verify:

- pump current draw
- MOSFET module rating and heat behavior
- power supply margin
- wiring and connector ratings
- whether a fuse or other protection should be added
