// Link the production local_run.cpp and rest_mode.cpp. Only clock, storage and
// physical modules are simulated; no duplicate implementation of run logic.
#include <cassert>
#include <iostream>
#include "Preferences.h"
#include "modules/local_run.h"
#include "modules/preset_store.h"
#include "modules/rest_mode.h"
#include "modules/grow_mode.h"

static unsigned long nowS = 1800000000UL;
static unsigned long nowMs = 0;
static bool synced = true, scheduleEnabled = true;
static bool pumpOn = false, lightOn = false, fanOn = false;
static int pumpStarts = 0;
static LocalPresetBlob preset = {};
unsigned long millis() { return nowMs; }
bool isTimeSynced() { return synced; }
unsigned long getEpochSeconds() { return synced ? nowS : 0; }
int getDateKey() { return nowS / 86400UL; }
int getHour() { return 8; }
int getMinute() { return 45; }
bool presetStore_loadSlot(int slot, LocalPresetBlob& out) { out = preset; return slot == 0; }
bool growMode_set(GrowMode, bool) { return true; }
bool climate_setTargets(float, float, int, int) { return true; }
bool light_setSchedule(int, int) { return true; }
void ui_setLightAuto() {}
void ui_setLightManualOff() { lightOn = false; }
void light_loop() { if (restMode_isEnabled()) lightOn = false; }
void fan_forceOff() { fanOn = false; }
void pumpScheduler_setAutoScheduleEnabled(bool enabled) { scheduleEnabled = enabled; }
void pumpScheduler_manualStop() { pumpOn = false; }
bool pumpScheduler_startAutoRunSeconds(int) { ++pumpStarts; pumpOn = true; return true; }

static void boot() { restMode_begin(); localRun_begin(); }
static void step(unsigned long seconds) { nowS += seconds; nowMs += 2000; localRun_loop(); }
static void parked(const char* status) {
  assert(!localRun_isActive());
  assert(restMode_isEnabled());
  assert(!scheduleEnabled && !pumpOn && !fanOn && !lightOn);
  assert(std::string(localRun_getStatusName()) == status);
}
static void start() { assert(localRun_start(0, nowS, "test-run", 1)); }

int main(int argc, char** argv) {
  assert(argc == 2);
  const std::string scenario = argv[1];
  preset.phase_count = 2;
  preset.checksum = 123;
  strcpy(preset.name, "Test grow");
  for (auto& phase : preset.phases) {
    strcpy(phase.key, "GROWTH"); strcpy(phase.label, "Growth");
    phase.duration_days = 1;
    phase.light_enabled = true;
    phase.day_temp_x10 = phase.night_temp_x10 = 230;
    phase.day_hum_pct = phase.night_hum_pct = 60;
  }
  boot();
  if (scenario == "boundary") {
    start(); step(86400); assert(localRun_getPhaseIndex() == 1);
    step(86399); assert(localRun_isActive());
    pumpOn = lightOn = fanOn = true;
    preset.phases[1].pump_enabled = true;
    preset.phases[1].pump_event_count = 1;
    preset.phases[1].pump_events[0] = {1, 8, 45, 5};
    step(1); parked("completed"); assert(pumpStarts == 0);
    assert(localRun_getTotalProgressPct() == 100);
  } else if (scenario == "manual_stop") {
    start(); pumpOn = lightOn = fanOn = true;
    assert(localRun_stop()); parked("stopped");
    step(86400); parked("stopped"); assert(localRun_stop());
  } else if (scenario == "reboot_completed") {
    start(); step(172800); parked("completed");
    synced = false; boot(); parked("completed");
    assert(std::string(localRun_getRunId()) == "test-run");
    assert(std::string(localRun_getPresetName()) == "Test grow");
  } else if (scenario == "reboot_stopped") {
    start(); assert(localRun_stop()); synced = false; boot(); parked("stopped");
  } else if (scenario == "pause_resume") {
    start(); step(3600); assert(localRun_pause(true));
    step(200000); assert(localRun_isActive()); assert(localRun_getRunAgeSeconds() == 3600);
    assert(localRun_pause(false)); step(169199); assert(localRun_isActive());
    step(1); parked("completed");
  } else if (scenario == "rest_elapsed") {
    start(); restMode_setEnabled(true, true); step(172800); parked("completed");
  } else if (scenario == "future_start") {
    assert(localRun_start(0, nowS + 86400, "test-run", 1));
    step(3600); assert(localRun_isActive()); assert(!localRun_isFinished());
    assert(pumpStarts == 0);
  } else if (scenario == "offline_time_wait") {
    start(); synced = false; step(200000); assert(localRun_isActive());
    synced = true; localRun_loop(); parked("completed");
  } else if (scenario == "new_explicit_start") {
    start(); step(172800); parked("completed");
    assert(!localRun_start(0, nowS, "test-run", 1));
    assert(localRun_start(0, nowS, "new-run", 2));
    assert(localRun_isActive() && !restMode_isEnabled());
    assert(localRun_getTotalProgressPct() == 0);
  } else if (scenario == "save_failure_retry") {
    start(); testNvs::failWrites = true; step(172800); parked("completed");
    String json; localRun_appendJson(json);
    assert(std::string(json.c_str()).find("\"terminal_persisted\":false") != std::string::npos);
    testNvs::failWrites = false; step(1); boot(); parked("completed");
  } else if (scenario == "power_loss_between_run_and_rest_save") {
    start(); testNvs::failKey = "vgrow-rest/enabled"; step(172800); parked("completed");
    testNvs::failKey.clear(); synced = false; boot(); parked("completed");
  } else if (scenario == "start_save_failure") {
    testNvs::failWrites = true; assert(!localRun_start(0, nowS, "test-run", 1));
    parked("stopped");
  } else if (scenario == "legacy_migration") {
    Preferences old; old.begin("grova-run");
    old.putBool("active", true);
    int8_t slot = 0; uint32_t startS = nowS - 172800;
    old.putBytes("slot", &slot, sizeof(slot)); old.putBytes("startS", &startS, sizeof(startS));
    old.putBytes("runId", "old-run", 8);
    boot(); localRun_loop(); parked("completed");
    assert(testNvs::data.count("grova-run/state") == 1);
  } else {
    assert(false && "unknown scenario");
  }
  std::cout << scenario << " OK\n";
}
