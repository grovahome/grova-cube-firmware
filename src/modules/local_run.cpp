#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

#include "modules/climate.h"
#include "modules/grow_mode.h"
#include "modules/fan.h"
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
static unsigned long pausedAtS = 0;
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
enum RunEnd : uint8_t { RUN_NOT_ENDED, RUN_COMPLETED, RUN_STOPPED };
static RunEnd runEnd = RUN_NOT_ENDED;
static unsigned long endedAtS = 0;
static bool terminalPersisted = true;
static unsigned long lastPersistAttempt = 0;

// One NVS blob prevents a reboot from combining a new active flag with an old
// run identity or completion marker. Older per-key records are read on upgrade.
struct StoredRun {
  uint32_t version;
  uint32_t startS, pauseS, endS, revision;
  int32_t slot, phase, day, pumpDate, pumpPhase;
  uint32_t pumpMask;
  float totalProgress, phaseProgress;
  uint8_t active, paused, end;
  char id[40], name[32], label[24];
};

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

static bool saveState() {
  if (!settingsReady) return false;
  StoredRun saved = {};
  saved.version = 1;
  saved.startS = startedAtS; saved.pauseS = pausedAtS; saved.endS = endedAtS;
  saved.revision = runRevision; saved.slot = runSlot;
  saved.phase = currentPhaseIndex; saved.day = runDay;
  saved.pumpDate = pumpEventDateKey; saved.pumpPhase = pumpEventPhaseIndex;
  saved.pumpMask = pumpEventMask;
  saved.totalProgress = totalProgressPct; saved.phaseProgress = phaseProgressPct;
  saved.active = runActive; saved.paused = runPaused; saved.end = runEnd;
  copyText(saved.id, sizeof(saved.id), runId);
  copyText(saved.name, sizeof(saved.name), presetName);
  copyText(saved.label, sizeof(saved.label), phaseLabel);
  return runSettings.putBytes("state", &saved, sizeof(saved)) == sizeof(saved);
}

static void parkOutputs() {
  restMode_setEnabled(true, false);
  pumpScheduler_setAutoScheduleEnabled(false);
  pumpScheduler_manualStop();
  fan_forceOff();
  light_loop();
}

static bool persistTerminal() {
  lastPersistAttempt = millis();
  const bool runSaved = saveState();
  const bool restSaved = restMode_setEnabled(true, true);
  terminalPersisted = runSaved && restSaved;
  return terminalPersisted;
}

static bool finishRun(RunEnd reason, unsigned long endTime) {
  parkOutputs();
  runActive = false;
  runPaused = false;
  pausedAtS = 0;
  runEnd = reason;
  endedAtS = endTime;
  copyText(phaseLabel, sizeof(phaseLabel), reason == RUN_COMPLETED ? "Complete" : "Stopped");
  if (reason == RUN_COMPLETED) {
    totalProgressPct = 100.0f;
    phaseProgressPct = 100.0f;
  }
  return persistTerminal();
}

static void loadState() {
  settingsReady = runSettings.begin("grova-run", false);
  if (!settingsReady) {
    Serial.println("Local run settings unavailable");
    runEnd = RUN_STOPPED;
    terminalPersisted = false;
    return;
  }
  if (runSettings.isKey("state")) {
    StoredRun saved = {};
    if (runSettings.getBytesLength("state") != sizeof(saved) ||
        runSettings.getBytes("state", &saved, sizeof(saved)) != sizeof(saved) ||
        saved.version != 1 || saved.end > RUN_STOPPED ||
        saved.active > 1 || saved.paused > 1 ||
        (saved.active && (saved.slot < 0 || saved.slot >= GROVA_PRESET_SLOT_COUNT || saved.startS == 0)) ||
        !memchr(saved.id, 0, sizeof(saved.id)) ||
        !memchr(saved.name, 0, sizeof(saved.name)) || !memchr(saved.label, 0, sizeof(saved.label))) {
      runActive = false;
      runEnd = RUN_STOPPED;
      terminalPersisted = false;
      return;
    }
    runEnd = static_cast<RunEnd>(saved.end);
    runActive = saved.active && runEnd == RUN_NOT_ENDED;
    runPaused = saved.paused && runActive;
    runSlot = saved.slot; currentPhaseIndex = saved.phase; runDay = saved.day;
    startedAtS = saved.startS; pausedAtS = saved.pauseS; endedAtS = saved.endS;
    runRevision = saved.revision;
    pumpEventDateKey = saved.pumpDate; pumpEventPhaseIndex = saved.pumpPhase;
    pumpEventMask = saved.pumpMask;
    totalProgressPct = saved.totalProgress; phaseProgressPct = saved.phaseProgress;
    copyText(runId, sizeof(runId), saved.id);
    copyText(presetName, sizeof(presetName), saved.name);
    copyText(phaseLabel, sizeof(phaseLabel), saved.label);
    return;
  }
  runActive = runSettings.getBool("active", false);
  runPaused = runSettings.getBool("paused", false);
  runSlot = runSettings.getChar("slot", -1);
  startedAtS = runSettings.getULong("startS", 0);
  pausedAtS = runSettings.getULong("pauseS", 0);
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
  if (runEnd != RUN_NOT_ENDED) {
    parkOutputs();
    persistTerminal();
  } else {
    pumpScheduler_setAutoScheduleEnabled(!runActive);
  }
  Serial.print("Local run active: ");
  Serial.println(runActive ? "yes" : "no");
}

void localRun_loop() {
  if (runEnd != RUN_NOT_ENDED) {
    parkOutputs();
    if (!terminalPersisted && millis() - lastPersistAttempt >= 1000UL) persistTerminal();
    return;
  }
  if (!runActive) {
    pumpScheduler_setAutoScheduleEnabled(true);
    return;
  }
  if (runPaused) {
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
    if (totalDurationS > 0 && ageS >= totalDurationS) {
      currentPhaseIndex = phaseIndex;
      runDay = totalDurationS / 86400UL;
      finishRun(RUN_COMPLETED, startedAtS + totalDurationS);
    } else {
      copyText(phaseLabel, sizeof(phaseLabel), "Waiting");
    }
    return;
  }
  if (restMode_isEnabled()) return;
  unsigned long phaseDurationS = static_cast<unsigned long>(phase->duration_days) * 86400UL;
  phaseProgressPct = phaseDurationS > 0 ? min(100.0f, ((nowS - phaseStartS) * 100.0f) / phaseDurationS) : 0.0f;
  applyPhase(*phase, phaseIndex, preset.checksum);
  runPumpEvents(*phase, phaseIndex);
}

bool localRun_settingsReady() { return settingsReady; }
bool localRun_isActive() { return runActive; }
bool localRun_isPaused() { return runPaused; }
bool localRun_isFinished() { return runEnd != RUN_NOT_ENDED; }
int localRun_getSlot() { return runSlot; }
int localRun_getPhaseIndex() { return currentPhaseIndex; }
int localRun_getDay() { return runDay; }
float localRun_getTotalProgressPct() { return totalProgressPct; }
float localRun_getPhaseProgressPct() { return phaseProgressPct; }
unsigned long localRun_getStartedAtSeconds() { return startedAtS; }
unsigned long localRun_getRunAgeSeconds() {
  unsigned long nowS = runEnd != RUN_NOT_ENDED ? endedAtS : (runPaused ? pausedAtS : getEpochSeconds());
  if (startedAtS == 0 || nowS <= startedAtS) return 0;
  return nowS - startedAtS;
}
uint32_t localRun_getRevision() { return runRevision; }
const char* localRun_getRunId() { return runId; }
const char* localRun_getPresetName() { return presetName; }
const char* localRun_getPhaseLabel() { return phaseLabel; }
const char* localRun_getStatusName() {
  if (runEnd == RUN_COMPLETED) return "completed";
  if (runEnd == RUN_STOPPED) return "stopped";
  if (!runActive) return "idle";
  if (restMode_isEnabled()) return "rest";
  if (runPaused) return "paused";
  if (!isTimeSynced()) return "time_wait";
  return "active";
}

bool localRun_start(int slot, unsigned long startAtSeconds, const char* newRunId, uint32_t revision) {
  if (!settingsReady) return false;
  if (revision > 0 && revision < runRevision) return false;
  if (revision > 0 && revision == runRevision) {
    // A repeated delivery must not restart a completed run or reset pump locks.
    return runActive && newRunId && strcmp(newRunId, runId) == 0 && slot == runSlot;
  }
  LocalPresetBlob preset;
  if (!presetStore_loadSlot(slot, preset)) return false;
  const unsigned long start = startAtSeconds > 0 ? startAtSeconds : getEpochSeconds();
  if (start == 0) return false;
  parkOutputs();
  runActive = true;
  runPaused = false;
  runEnd = RUN_NOT_ENDED;
  endedAtS = 0;
  terminalPersisted = true;
  runSlot = slot;
  startedAtS = start;
  pausedAtS = 0;
  runRevision = revision > 0 ? revision : runRevision + 1;
  currentPhaseIndex = -1;
  appliedPresetChecksum = 0;
  pumpEventDateKey = -1;
  pumpEventPhaseIndex = -1;
  pumpEventMask = 0;
  runDay = 0;
  totalProgressPct = 0;
  phaseProgressPct = 0;
  copyText(runId, sizeof(runId), newRunId && strlen(newRunId) ? newRunId : "local-run");
  copyText(presetName, sizeof(presetName), preset.name);
  copyText(phaseLabel, sizeof(phaseLabel), "Starting");
  pumpScheduler_setAutoScheduleEnabled(false);
  if (!saveState() || !restMode_setEnabled(false, true)) {
    finishRun(RUN_STOPPED, getEpochSeconds());
    return false;
  }
  localRun_loop();
  return true;
}

bool localRun_stop() {
  if (runEnd != RUN_NOT_ENDED) {
    parkOutputs();
    return persistTerminal();
  }
  return finishRun(RUN_STOPPED, getEpochSeconds());
}

bool localRun_pause(bool paused) {
  if (!settingsReady || !runActive) return false;
  if (paused == runPaused) return true;
  if (!isTimeSynced()) return false;
  unsigned long nowS = getEpochSeconds();
  if (nowS == 0 || (!paused && (pausedAtS == 0 || nowS < pausedAtS))) return false;
  const unsigned long previousStart = startedAtS;
  const unsigned long previousPause = pausedAtS;
  if (paused && !runPaused) {
    pausedAtS = nowS;
    pumpScheduler_manualStop();
  } else if (!paused && runPaused) {
    if (isTimeSynced() && pausedAtS > 0 && nowS > pausedAtS) {
      startedAtS += nowS - pausedAtS;
    }
    pausedAtS = 0;
  }
  runPaused = paused;
  if (!saveState()) {
    startedAtS = previousStart;
    pausedAtS = previousPause;
    runPaused = !paused;
    // A failed resume stays paused. A failed pause parks the whole run and
    // retries terminal persistence rather than silently continuing watering.
    if (paused) finishRun(RUN_STOPPED, nowS);
    return false;
  }
  if (!runPaused) localRun_loop();
  return true;
}

void localRun_appendJson(String& json) {
  json += "\"local_run\":{";
  appendJsonBool(json, "completion_rest", true);
  appendJsonBool(json, "terminal_persisted", terminalPersisted);
  appendJsonInt(json, "ended_at_s", endedAtS);
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
  appendJsonInt(json, "paused_at_s", pausedAtS);
  appendJsonInt(json, "age_s", localRun_getRunAgeSeconds());
  appendJsonFloat(json, "total_progress_pct", totalProgressPct, 1);
  appendJsonFloat(json, "phase_progress_pct", phaseProgressPct, 1);
  appendJsonInt(json, "pump_event_date_key", pumpEventDateKey);
  appendJsonInt(json, "pump_event_phase_index", pumpEventPhaseIndex);
  appendJsonInt(json, "pump_event_mask", pumpEventMask, false);
  json += "}";
}
