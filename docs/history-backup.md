# RAM measurement history and equipment backup

## Measurement history

The overview offers a lazy-loaded canvas chart via GET /api/v1/history. The chart
selects a concrete sensor source/measurement or one of the seven physical output
actuation series. It never silently merges same-address sensors on different mux
channels. Invalid/missing measurements are null gaps, not zero values or lines
bridging those gaps. Y axes use one unit per selection; 0..100% bounds apply to
normal percentage series. Offline/failed refreshes are labelled last loaded.

The fixed RAM ring has 120 captures, at most one per 30 seconds, about one hour.
There is no synthetic backfill. Timestamps retain UTC only when valid; plotting
uses monotonic capture order and handles millis wrap. At most 64 series, including
seven reserved output series, are retained. Series with no valid values left in
the window are pruned; omitted series count is visible. Output values are driver
actuation setpoints, not measured voltage/light/flow. Short pump impulses can fall
between captures; the durable watering journal remains the evidence for those runs.

Only the open chart in the overview refreshes every 30 s. History is not added to
the frequent status response. Serialization is bounded and writes directly into
a reserved response String, avoiding a full 64x120 ArduinoJson DOM. The history
contains no Preferences, filesystem or flash writes and resets on reboot/OTA.

## Equipment backup

GET /api/v1/configuration exports JSON format grova-core-equipment, version 1,
hardware_profile and a schema-1 equipment configuration. Device/port assignment,
mode, fan function, plans, sensor rules and device protection settings are included.
WLAN/MQTT credentials, instantaneous output/manual demand, operation intent, time
history, event history and watering/command journals are excluded. Pulse creation
baseline tokens are not transported; the target board establishes safe baselines.
This is a device/rule backup, not a full flash image or hardware calibration dump.

The browser downloads the file to the phone/PC without writing ESP flash. Import
has a file-size/format/hardware preview and device mapping summary; malformed or
superseded asynchronous selections are rejected. The Restore button is explicit
and guarded by the current equipment revision. The request is local-only through
the existing protected control route (restore_equipment); setup/OTA and remote
MQTT control restrictions remain in place. File/request limit is 16 KiB and the
canonical stored configuration remains bounded to 12 KiB.

The backend first dry-validates the complete configuration using the production
profile/voltage/assignment/plan/rule/protection parser, without NVS writes or output
changes. It then durably pauses through the existing operation contract and saves
the configuration. Imported start_plans or operation fields cannot release outputs.
After success, and after reboot, operation stays paused until explicitly resumed.
Configuration write failure retains the previous assignment and Pause; inability
to persist Pause prevents the restore. Stale revision or malformed/unsafe input is
rejected before pausing. Lost acknowledgements are reconciled by revision/status,
never automatic command replay. An explicit restore necessarily writes NVS; normal
plotting, downloads and event recording do not. Durable watering protection remains.

## Verification

Pure history tests cover sample timing, source isolation, gaps, overwrite/prune,
series cap, restart and millis wrap. Host API tests cover secret-free export,
validation before pause/write, wrong board/voltage/revision, remote denial, paused
restore and fresh-process restart, failed write and RAM-only sampling. Browser
suites check canvas gap handling, source retention, stale/offline/reboot display,
JSON download, detailed file preview, malformed selections, explicit restore,
revision conflicts, failed writes, lost ACK without replay and mobile layout.
No backup is restored on the user's live board during acceptance; new live GET
endpoints and plotting/export are checked read-only. Hardware response still
requires physical sensor/load acceptance.
