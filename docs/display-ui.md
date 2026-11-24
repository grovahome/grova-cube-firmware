# OLED and rotary encoder

Use the OLED to check GROVA and operate configured devices from the enclosure.
This guide covers the current [GROVA Core ESP32-S3 firmware](firmware-profiles.md).
The frozen [GROVA V1 branch](v1-freeze.md) has its own display guide.

## Using the knob

- Turn to move the highlight or change the value on an adjustment screen.
- Press to open the highlighted item or confirm the action in the footer.
- Hold for about one second and release to return to the home screen. You can
  also select **Back to Home** from the main menu.
- If the screen is off or showing the saver, the first turn or press wakes it.
  Use a second input to select an item or change a value.

The home screen shows the clock, **ACTIVE** or **PAUSED**, and the current
temperature and humidity. When a timer action is scheduled, the bottom row
shows its time, action and device. While paused, it shows **Schedule paused**.
A sensor outage takes priority on this row. The web overview shows the full
action with **Heute** (today) or **Morgen** (tomorrow).

Press the knob to open the main menu.

| Menu | Use it to |
| --- | --- |
| **Climate** | Check the active sensor, its port, the age of its last measurement, and available temperature, humidity and pressure readings. |
| **Operation** | Pause all devices or review which automation will resume. |
| **Devices** | Check and operate devices already assigned in the web interface. |
| **Settings** | Set **Idle Mode** and **Brightness** or open **Network** to find the board IP. |
| **Service** | Check **Outputs** and **Fans**, or use **Output Test** on an individual port. |

**Sensor offline** means GROVA previously received a temperature reading and
the source is now unavailable. **Climate** keeps the last known source and
shows the age of its last sample. The warning clears when the signal returns.

## Pause or resume the controller

1. Open **Operation** and select **Pause all** or **Resume**.
2. To pause, review **All outputs off**. To resume, review the listed schedules
   and sensor-controlled devices. **Cancel** returns to the Operation menu.
3. Turn to **Confirm** and press. If the device configuration changes during
   the review, open **Resume** again to review the current assignments.

## Operate a device

1. Open **Devices** and choose an assigned device.
2. For a manual light or fan, turn to choose **ON**, **OFF**, or a percentage,
   depending on the device. Press when the footer shows **Press Save**.
3. For a scheduled or sensor-controlled device, saving the value starts an
   override for up to 15 minutes. The display shows the time left. To return
   to automation early, press when the footer shows **Press Auto**.
   Adjusting a running override keeps its original end time. Pausing ends it.

A pump or other timed device shows **RUNNING** or **STOPPED**. While it is running,
turn toward **Stop** and press to confirm. Stopping an automated timed device
uses **Pause all**, so the other automatic outputs pause too.

Use the web interface to assign devices and change schedules or sensor rules.
Timed starts also stay in the web interface. See [equipment and port assignment](equipment.md).

## Set brightness or idle behavior

Open **Settings → Brightness** and choose **Low**, **Medium** or **High**. Under
**Settings → Idle Mode**, choose what happens after one minute without encoder
input.

| Idle mode | Behavior |
| --- | --- |
| **Screen Off** | Turns the OLED off. |
| **GROVA Saver** | Moves the GROVA wordmark across the screen and bounces it off the edges. |
| **Always On** | Leaves the current view visible. |

Press to save the selected option. An asterisk marks the saved choice.

## Test a physical output

1. Confirm the controller shows **PAUSED**, then open **Service → Output Test**.
2. Select a fan, a free 5 V or 12 V output, or a free 0–10 V output. Assigned
   switch and 0–10 V outputs remain under their configured device controls.
3. Turn to select **ON**, a fan percentage or a voltage setpoint. Press when
   the footer shows **Press Run**. During the test, turn to adjust the level or
   press to stop. The test ends after ten seconds.

Only one port is tested at a time. The voltage shown is the commanded setpoint.
