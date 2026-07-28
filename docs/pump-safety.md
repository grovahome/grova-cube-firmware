# Pump Safety

Pump safety applies to automatic pump runs and local preset pump events. Manual pump tests are available for setup and maintenance, but Rest Mode and the hard runtime limit still apply.

## Automatic Pump Event Lock

```text
Scheduled event lock: once per date/minute event
Startup lock after reboot: 10 minutes
Hard maximum pump runtime: 10 seconds
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

## Pump Tests

Pump tests:

- do not count as scheduled pump events
- are not blocked by the startup lock
- are blocked by Rest Mode
- are still limited by the hard 10-second runtime limit

## Grow Mode Interaction

```text
GERMINATION: automatic pump events remain active
GROWTH:      automatic pump events remain active
HARVEST:     automatic pump events are blocked
```

## Production Notes

Before long-term unattended use, verify:

- pump current draw
- MOSFET module rating and heat behavior
- power supply margin
- wiring and connector ratings
- whether a fuse or other protection should be added