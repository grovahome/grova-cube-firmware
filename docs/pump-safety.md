# Pump Safety

Pump safety applies to automatic pump runs. Manual pump tests are intentionally available for setup and maintenance, but still obey the hard runtime limit.

## Automatic Pump Limits

```text
Maximum automatic pump runs per day: 2
Minimum interval between automatic runs: 6 hours
Startup lock after reboot: 10 minutes
Hard maximum pump runtime: 10 seconds
Automatic pump duration: configurable from 1 to 10 seconds
```

## Pump Tests

Pump tests:

- do not count toward the daily automatic run limit
- do not count toward the minimum automatic interval
- are not blocked by the startup lock
- are still limited by the hard 10-second runtime limit

## Grow Mode Interaction

```text
GERMINATION: automatic pump remains active
GROWTH:      automatic pump remains active
HARVEST:     automatic pump is blocked
```

Manual pump test remains available in harvest mode.

## Production Notes

Before long-term unattended use, verify:

- pump current draw
- MOSFET module rating and heat behavior
- power supply margin
- wiring and connector ratings
- whether a fuse or other protection should be added
