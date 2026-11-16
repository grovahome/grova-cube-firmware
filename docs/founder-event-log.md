# Founder RAM event history

The local overview exposes the newest 64 sensor, limit and operation events of the
current boot. The fixed ring overwrites the oldest entry once full, with count,
total and overwritten counters. Entries copy the device name, source and rule
priority and applicable limit at event time so later edits/removal do not reinterpret old records.
Events are ordered by a monotonically increasing per-boot sequence, not wall time.
UTC is recorded only if valid; otherwise uptime is shown explicitly without an
invented date. The browser can download a JSON snapshot without any ESP filesystem
write. Offline snapshots are labelled last received.

Recorded edges: boot, successful configuration save, initial operation state and
Pause/release changes, selected measurement unavailable/recovered, debounced and
hysteretic minimum/maximum warning transitions, fallback required/active/ended.
Normal samples, every percent adjustment and repeated reads are not recorded.
Monitoring is independent of actuation and also works during Pause; blocked
fallback is labelled required rather than active. Sensor loss never emits a false
limit-recovery event. Configuration saves reset observer baselines because rule
identities/thresholds may have changed. This is an operator history, not a complete
actuator-run audit; bounded watering evidence remains in the existing durable journal.

## Endurance and persistence

The new logger uses only a fixed RAM array and bounded observer state. It has no
Preferences, NVS, SPIFFS/FAT or flash write call. Runtime events therefore add no
flash write/erase cycles; reads/status polls and JSON downloads do not persist them.
RAM contents are lost on power interruption, reboot or OTA. Durable long-term
history should eventually be collected externally, rather than written per sample
on the ESP. This version adds no server deployment.

Existing durability remains essential and unchanged: founder-devices/configuration
is saved on explicit Save; operation/Pause stores restart intent; plan-ledger
reserves/records automatic watering; the existing NVS key named events stores timed
command duplicate protection and is NOT this RAM history. Safety-critical records
must survive interruption and are not moved into RAM. NVS performs erase/write
balancing, but endurance depends on the actual flash device, allocation and traffic;
a putBytes call is not a guaranteed one-to-one physical sector erase. Do not infer
a board lifetime from a generic cycle number or the single-entry ideal case.
Reference: https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/storage/nvs.html

## Verification

Pure ring tests cover bounded overwrite, copied data, repeated-value suppression,
missing/recovery, limit hysteresis/debounce, fallback states, reboot reset and millis
wrap. API integration compares NVS and output state across repeated observation and
status reads. Browser acceptance verifies history display, escaped text, unknown
clock, JSON download, reboot replacement, offline labelling, mobile layout and no
control POSTs. Installed binary/source and live checks are recorded separately.


The recent-events UI shows timestamp and text only; navigation to rule diagnostics
remains available in current warnings. A transient failed sensor read does not
create loss/recovery events while its last successful exact-source value is less
than ten seconds old. At ten seconds it becomes unavailable and normal deduplicated
loss/fallback events apply. Never-seen sources remain unavailable immediately.
