# Local Wi-Fi setup

Status: 2026-10-06. OTA-installed on Cube 002 / PCB v1 only, on the
`grova-core-v1` line. Cube 001 stays frozen.

## User flow

1. With no valid saved or compiled network, the Cube starts a protected
   `GROVA-XXXXXX` access point. Pump, light and fans stay in Rest Mode.
2. The OLED remains awake and displays the setup SSID, its 12-character
   password and the actual AP IP (normally `192.168.4.1`). Connect a phone to
   this network, accept staying connected without Internet, then open that
   IP using HTTP. Captive-portal detection may open the page automatically.
3. Enter the same setup password on the page, scan for 2.4-GHz networks or
   enter an SSID manually, then enter the home Wi-Fi password.
4. The Cube tests the connection for up to 30 seconds. A connection with an
   assigned IP must remain up for two seconds before credentials are saved.
   Internet, NTP and the GROVA backend are not required for this test.
5. The page and OLED show the station IP. Rejoin the home network and open
   that IP to use the existing ESP webapp. The setup AP closes after one
   minute of station connectivity. Initial setup leaves Rest Mode enabled.

The home Wi-Fi password is never displayed on the OLED or returned by the API.
The setup password is generated once and saved in NVS; it can later be printed
on a device leaflet. It is also available on the OLED System page for later
network changes through `/wifi`. Without an OLED, the device-specific setup
password must be supplied separately. Leaflet production is not implemented.

## Persistence and recovery

- A valid saved record takes precedence over compiled `WIFI_SSID` / `WIFI_PASS`.
  Compiled credentials remain a migration fallback for existing devices only
  when no saved record exists. New-device builds should leave them empty.
- Invalid saved records trigger setup, rather than silently selecting an old
  compiled network. Firmware upload alone does not clear existing NVS settings.
- Candidate credentials stay in RAM until validated. Timeout, storage failure
  or reboot during testing preserves the previous saved network. Failed tests
  reconnect to it automatically.
- After 60 seconds without Wi-Fi, the setup AP becomes available for recovery.
  An ordinary network outage does not put an existing run into Rest or reset it.
  Losing Internet while still connected to Wi-Fi does not start setup.
- Station reconnection retries every 30 seconds. Scanning is asynchronous,
  limited to once per 10 seconds, and blocks credential submission while active.
  Failed AP starts retry no faster than once per 10 seconds.
- AP and station share the ESP radio. Joining a different channel can briefly
  disconnect the setup client; reconnect if needed.

## Scope and security

Setup handles Wi-Fi only. Core naming, timezone, server pairing, factory reset,
enterprise Wi-Fi and additional settings are outside this change.

The AP uses a per-device random password. Scan and credential-write endpoints
require that password in `X-GROVA-Setup-Key`, with a 30-second lockout after five
failed attempts. No CORS permission is granted. Responses contain no passwords.
SSID rendering uses text nodes and JSON escaping. AP HTTP clients cannot invoke
the existing control/configuration write routes. Initial setup also blocks MQTT
control and encoder actions; normal operation resumes only after configuration.

This is local HTTP and ordinary NVS storage, not HTTPS or encrypted flash. The
home LAN and people with physical access to the OLED remain trusted. Existing
LAN webapp/OTA access controls are unchanged. Record checksums detect corruption;
they do not provide encryption. A complete NVS erase regenerates the setup key
and invalidates any previously printed leaflet.

## Validation

- `python tests/host/run_tests.py`: 25 lifecycle scenarios, 16 Wi-Fi scenarios
  and the HTTP integration harness pass against production module code with
  mocked radio, time, transport and storage adapters.
- Coverage includes first boot, compiled migration, saved precedence, failed
  replacement, write errors, reboot during a trial, DHCP requirement, invalid
  records, recovery without run reset, scan conflict, timer wrap, failed AP start,
  setup-key persistence failure, authorization/rate limiting and SSID escaping.
- Embedded JavaScript checked with Node: parsing, form submission/header,
  password-field clearing, state changes, station link and literal SSID rendering.
- `pio run -e grova_core_v1`: successful; RAM 56,192 bytes, flash 1,033,593 bytes.

Open hardware checks: OLED legibility; Android/iOS captive-portal behaviour;
real scan/channel changes; wrong password and missing DHCP; power loss during
NVS writes; reconnect after router restart; outputs off during first setup;
continued local run during network loss. These physical acceptance checks remain open.

## OTA verification 2026-10-06

Transferred the verified application to Cube 002 at `192.168.1.97:3232` using
`espota.py`; upload succeeded. Firmware reports build `Oct 6 2026 10:26:40`,
environment `grova_core_v1`, healthy/settings OK, synchronized time and Wi-Fi
connected after reboot. Rest remains enabled, the local run is idle, and
pump/light/both fans remain off. Existing Wi-Fi configuration was retained.
`/wifi` returns HTTP 200 with the setup form and no-store caching; `/api/v1/wifi`
reports connected, storage ready, no error and no running credential test.

Uploaded application SHA-256:
`851a506d123ff35c2fb793d3cafdc70e374fd5969be5187c903fc8de7f3908c8`.

This release also deploys the previously committed pump reservation and invalid
preset safeguards from `b1ad951`. The server dashboard was not deployed here.
No WLAN credentials were changed to force onboarding. Cube 002 currently detects
no OLED, so display/phone/AP onboarding acceptance awaits suitable hardware.
