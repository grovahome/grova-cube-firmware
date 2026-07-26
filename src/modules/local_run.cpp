#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

#include "modules/climate.h"
#include "modules/grow_mode.h"
#include "modules/light.h"
#include "modules/local_run.h"
#include "modules/preset_store.h"
#include "modules/pump_scheduler.h"
#include "modules/rest_mode.h"
#include "modules/time_sync.h"
#include "modules/ui.h"

static Preferences runSettings;
static bool settingsReady = false;
static bool runActive = false;
static bool runPaused = false;
static int runSlot = -1;
static int currentPhaseIndex = -1;
static int runDay = 0;
static unsigned long startedAtS = 0;
static uint32_t runRevision = 0;
static char runId[40] = "";
static char presetName[32] = "";
static char phaseLabel[24] = "";
static int pumpEventDateKey = -1;
static int pumpEventPhaseIndex = -1;
static uint32_t pumpEventMask = 0;
static uint32_t appliedPresetChecksum = 0;
static float totalProgressPct = 0.0;
static float phaseProgressPct = 0.0;

static void appendEscapedJsonValue(String& json, const char* value) {
  for (const char* cursor = value; *cursor; cursor++) {
    char c = *cursor;
    if (c == '"' || c == '\\') {
      json += '\\';
      json += c;
    } else {
      json += c;
    }
  }
}

static void appendJsonString(String& json, const char* key, const char* value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  appendEscapedJsonValue(json, value);
  json += "\"";
  if (comma) json += ",";
}

static void appendJsonBool(String& json, const char* key, bool value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (comma) json += ",";
}

static void appendJsonInt(String& json, const char* key, long value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value;
  if (comma) json += ",";
}

static void appendJsonFloat(String& json, const char* key, float value, int decimals, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += String(value, decimals);
  if (comma) json += ",";
}

static void copyText(char* target, size_t targetSize, const char* source) {
  if (targetSize == 0) return;
  strncpy(target, source ? source : "", targetSize - 1);
  target[targetSize - 1] = '\0';
}

static const LocalPresetPhase* activePhase(const LocalPresetBlob& preset, unsigned long nowS, int& phaseIndexOut, unsigned long& phaseStartOut, unsigned long& totalDurationOut) {
  phaseIndexOut = -1;
  phaseStartOut = startedAtS;
  totalDurationOut = 0;
  if (!runActive || startedAtS == 0 || nowS < startedAtS || preset.phase_count == 0) return nullptr;

  unsigned long cursor = startedAtS;
  unsigned long elapsed = nowS - startedAtS;
  for (uint8_t i = 0; i < preset.phase_count; i++) {
    totalDurationOut += static_cast<unsigned long>(preset.phases[i].duration_days) * 86400UL;
  }
  for (uint8_t i = 0; i < preset.phase_count; i++) {
    unsigned long phaseDuration = static_cast<unsigned long>(preset.phases[i].duration_days) * 86400UL;
    if (phaseDuration == 0) continue;
    if (elapsed < (cursor - startedAtS) + phaseDuration) {
      phaseIndexOut = i;
      phaseStartOut = cursor;
      return &preset.phases[i];
    }
    cursor += phaseDuration;
  }
  phaseIndexOut = preset.phase_count - 1;
  phaseStartOut = cursor;
  return nullptr;
}

static GrowMode phaseGrowMode(const LocalPresetPhase& phase) {
  if (strcmp(phase.key, "GERMINATION") == 0 || strcmp(phase.key, "BLACKOUT") == 0) return GROW_MODE_GERMINATION;
  if (strcmp(phase.key, "HARVEST") == 0) return GROW_MODE_HARVEST;
  return GROW_MODE_GROWTH;
}

static void applyPhase(const LocalPresetPhase& phase, int phaseIndex, uint32_t presetChecksum) {
  if (phaseIndex == currentPhaseIndex && presetChecksum == appliedPresetChecksum) return;
  currentPhaseIndex = phaseIndex;
  appliedPresetChecksum = presetChecksum;
  copyText(phaseLabel, sizeof(phaseLabel), phase.label);
  growMode_set(phaseGrowMode(phase), true);
  climate_setTargets(
    phase.day_temp_x10 / 10.0,
    phase.night_temp_x10 / 10.0,
    phase.day_hum_pct,
    phase.night_hum_pct
  );
  if (phase.light_enabled) {
    light_setSchedule(phase.light_on_hour, phase.light_off_hour);
    ui_setLightAuto();
  } else {
    ui_setLightManualOff();
  }
  Serial.print("Local run phase applied: ");
  Serial.println(phase.label);
}

static void saveState() {
  if (!settingsReady) return;
  runSettings.putBool("active", runActive);
  runSettings.putBool("paused", runPaused);
  runSettings.putChar("slot", static_cast<int8_t>(runSlot));
  runSettings.putULong("startS", startedAtS);
  runSettings.putUInt("rev", runRevision);
  runSettings.putString("runId", runId);
  runSettings.putInt("pumpDate", pumpEventDateKey);
  runSettings.putInt("pumpPhase", pumpEventPhaseIndex);
  runSettings.putUInt("pumpMask", pumpEventMask);
}

static void loadState() {
  settingsReady = runSettings.begin("grova-run", false);
  if (!settingsReady) {
    Serial.println("Local run settings unavailable");
    return;
  }
  runActive = runSettings.getBool("active", false);
  runPaused = runSettings.getBool("paused", false);
  runSlot = runSettings.getChar("slot", -1);
  startedAtS = runSettings.getULong("startS", 0);
  runRevision = runSettings.getUInt("rev", 0);
  String storedRunId = runSettings.getString("runId", "");
  copyText(runId, sizeof(runId), storedRunId.c_str());
  pumpEventDateKey = runSettings.getInt("pumpDate", -1);
  pumpEventPhaseIndex = runSettings.getInt("pumpPhase", -1);
  pumpEventMask = runSettings.getUInt("pumpMask", 0);
  if (runActive && (runSlot < 0 || startedAtS == 0)) runActive = false;
}

static void runPumpEvents(const LocalPresetPhase& phase, int phaseIndex) {
  if (!phase.pump_enabled || phase.pump_event_count == 0 || !isTimeSynced()) return;
  int dateKey = getDateKey();
  if (dateKey < 0) return;
  if (dateKey != pumpEventDateKey || phaseIndex != pumpEventPhaseIndex) {
    pumpEventDateKey = dateKey;
    pumpEventPhaseIndex = phaseIndex;
    pumpEventMask = 0;
    saveState();
  }

  for (uint8_t index = 0; index < phase.pump_event_count; index++) {
    if (pumpEventMask & (1UL << index)) continue;
    const LocalPresetPumpEvent& event = phase.pump_events[index];
    if (!event.enabled) {
      pumpEventMask |= (1UL << index);
      continue;
    }
    if (getHour() == event.hour && getMinute() == event.minute) {
      if (pumpScheduler_startAutoRunSeconds(event.duration_s)) {
        pumpEventMask |= (1UL << index);
        saveState();
        Serial.print("Local run pump event started ");
        Serial.println(index + 1);
      }
      return;
    }
  }
}

void localRun_begin() {
  loadState();
  LocalPresetBlob preset;
  if (runActive && presetStore_loadSlot(runSlot, preset)) {
    copyText(presetName, sizeof(presetName), preset.name);
  }
  pumpScheduler_setAutoScheduleEnabled(!runActive);
  Serial.print("Local run active: ");
  Serial.println(runActive ? "yes" : "no");
}

void localRun_loop() {
  if (!runActive) {
    pumpScheduler_setAutoScheduleEnabled(true);
    return;
  }
  if (runPaused || restMode_isEnabled()) {
    pumpScheduler_setAutoScheduleEnabled(false);
    return;
  }
  pumpScheduler_setAutoScheduleEnabled(false);
  if (!isTimeSynced()) return;
  LocalPresetBlob preset;
  if (!presetStore_loadSlot(runSlot, preset)) {
    copyText(phaseLabel, sizeof(phaseLabel), "Missing preset");
    return;
  }
  copyText(presetName, sizeof(presetName), preset.name);
  unsigned long nowS = getEpochSeconds();
  unsigned long phaseStartS = 0;
  unsigned long totalDurationS = 0;
  int phaseIndex = -1;
  const LocalPresetPhase* phase = activePhase(preset, nowS, phaseIndex, phaseStartS, totalDurationS);
  unsigned long ageS = nowS > startedAtS ? nowS - startedAtS : 0;
  runDay = static_cast<int>(ageS / 86400UL) + 1;
  totalProgressPct = totalDurationS > 0 ? min(100.0f, (ageS * 100.0f) / totalDurationS) : 0.0f;
  if (!phase) {
    copyText(phaseLabel, sizeof(phaseLabel), totalDurationS > 0 && ageS >= totalDurationS ? "Complete" : "Waiting");
    return;
  }
  unsigned long phaseDurationS = static_cast<unsigned long>(phase->duration_days) * 86400UL;
  phaseProgressPct = phaseDurationS > 0 ? min(100.0f, ((nowS - phaseStartS) * 100.0f) / phaseDurationS) : 0.0f;
  applyPhase(*phase, phaseIndex, preset.checksum);
  runPumpEvents(*phase, phaseIndex);
}

bool localRun_settingsReady() { return settingsReady; }
bool localRun_isActive() { return runActive; }
bool localRun_isPaused() { return runPaused; }
int localRun_getSlot() { return runSlot; }
int localRun_getPhaseIndex() { return currentPhaseIndex; }
int localRun_getDay() { return runDay; }
float localRun_getTotalProgressPct() { return totalProgressPct; }
float localRun_getPhaseProgressPct() { return phaseProgressPct; }
unsigned long localRun_getStartedAtSeconds() { return startedAtS; }
unsigned long localRun_getRunAgeSeconds() {
  unsigned long nowS = getEpochSeconds();
  if (!runActive || nowS <= startedAtS) return 0;
  return nowS - startedAtS;
}
uint32_t localRun_getRevision() { return runRevision; }
const char* localRun_getRunId() { return runId; }
const char* localRun_getPresetName() { return presetName; }
const char* localRun_getPhaseLabel() { return phaseLabel; }
const char* localRun_getStatusName() {
  if (!runActive) return "idle";
  if (restMode_isEnabled()) return "rest";
  if (runPaused) return "paused";
  if (!isTimeSynced()) return "time_wait";
  return "active";
}

bool localRun_start(int slot, unsigned long startAtSeconds, const char* newRunId, uint32_t revision) {
  if (!settingsReady) return false;
  if (revision > 0 && revision < runRevision) return false;
  LocalPresetBlob preset;
  if (!presetStore_loadSlot(slot, preset)) return false;
  runActive = true;
  runPaused = false;
  runSlot = slot;
  startedAtS = startAtSeconds > 0 ? startAtSeconds : getEpochSeconds();
  runRevision = revision > 0 ? revision : runRevision + 1;
  currentPhaseIndex = -1;
  appliedPresetChecksum = 0;
  pumpEventDateKey = -1;
  pumpEventPhaseIndex = -1;
  pumpEventMask = 0;
  copyText(runId, sizeof(runId), newRunId && strlen(newRunId) ? newRunId : "local-run");
  copyText(presetName, sizeof(presetName), preset.name);
  copyText(phaseLabel, sizeof(phaseLabel), "Starting");
  pumpScheduler_setAutoScheduleEnabled(false);
  saveState();
  localRun_loop();
  return true;
}

bool localRun_stop() {
  if (!settingsReady) return false;
  runActive = false;
  runPaused = false;
  runSlot = -1;
  startedAtS = 0;
  currentPhaseIndex = -1;
  appliedPresetChecksum = 0;
  runDay = 0;
  totalProgressPct = 0;
  phaseProgressPct = 0;
  pumpEventDateKey = -1;
  pumpEventPhaseIndex = -1;
  pumpEventMask = 0;
  copyText(phaseLabel, sizeof(phaseLabel), "");
  pumpScheduler_setAutoScheduleEnabled(true);
  pumpScheduler_manualStop();
  saveState();
  return true;
}

bool localRun_pause(bool paused) {
  if (!settingsReady || !runActive) return false;
  runPaused = paused;
  if (runPaused) pumpScheduler_manualStop();
  saveState();
  return true;
}

void localRun_appendJson(String& json) {
  json += "\"local_run\":{";
  appendJsonBool(json, "active", runActive);
  appendJsonBool(json, "paused", runPaused);
  appendJsonString(json, "status", localRun_getStatusName());
  appendJsonInt(json, "slot", runSlot);
  appendJsonString(json, "run_id", runId);
  appendJsonInt(json, "revision", runRevision);
  appendJsonString(json, "preset_name", presetName);
  appendJsonString(json, "phase_label", phaseLabel);
  appendJsonInt(json, "phase_index", currentPhaseIndex);
  appendJsonInt(json, "day", runDay);
  appendJsonInt(json, "started_at_s", startedAtS);
  appendJsonInt(json, "age_s", localRun_getRunAgeSeconds());
  appendJsonFloat(json, "total_progress_pct", totalProgressPct, 1);
  appendJsonFloat(json, "phase_progress_pct", phaseProgressPct, 1);
  appendJsonInt(json, "pump_event_date_key", pumpEventDateKey);
  appendJsonInt(json, "pump_event_phase_index", pumpEventPhaseIndex);
  appendJsonInt(json, "pump_event_mask", pumpEventMask, false);
  json += "}";
}
