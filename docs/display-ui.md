# Display And Encoder UI

The cube can be operated locally through the OLED display and rotary encoder. The ESP web UI and APIs are fallbacks and integration surfaces; the device should still be useful without a server.

## Display Pages

```text
STATUS
GROW
FAN
CLIMATE
LIGHT
PUMP
SENSORS
DIAG
SYSTEM
```

## Encoder Controls

```text
Rotate:      change page, or change value while editing
Short press: select page-specific field or action
Long press:  start editing or switch mode, depending on page
```

## Important Pages

### Grow

- Short press cycles and saves the grow mode.
- Long press starts edit mode, rotate selects mode, short press saves.

### Fan

- Short press starts/stops manual speed editing.
- Long press switches between automatic and manual mode.

### Climate

- Short press selects target value.
- Long press starts edit mode.
- Values are saved persistently.

### Light

- Short press selects state/on/off fields.
- Long press on state switches automatic/manual off/manual on.
- Long press on schedule fields starts editing.

### Pump

- Short press selects test/hour/minute/duration fields.
- Long press on test starts or stops a pump test.
- Long press on hour/minute/duration starts editing.
- Automatic pump duration is clamped to `1..10` seconds.

## Display Sleep

The display sleeps after 60 seconds. The first encoder action wakes the display and does not change a value.
