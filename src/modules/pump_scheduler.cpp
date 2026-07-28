#include <Arduino.h>
#include <Preferences.h>

#include "config.h"
#include "modules/grow_mode.h"
#include "modules/rest_mode.h"
#include "modules/time_sync.h"
#include "modules/pump.h"
#include "modules/pump_scheduler.h"

static bool pumpRunning = false;
static unsigned long pumpStartTime = 0;
static unsigned long pumpRuntimeMs = 0;
static PumpMode pumpMode = PUMP_MODE_IDLE;

static int lastRunDateKey = -1;
static int lastRunDay = -1;
static int lastRunMinuteOfDay = -1;
static int autoRunCountDateKey = -1;
static int autoRunCount = 0;
static Preferences pumpSettings;
static bool pumpSettingsReady = false;
static bool pumpSettingsDirty = false;
static bool autoScheduleEnabled = true;

static int pumpRunHour = PUMP_RUN_HOUR;
static int pumpRunMinute = PUMP_RUN_MINUTE;
static int pumpRunDurationSeconds = PUMP_RUNTIME_MS / 1000UL;

static int wrapHour(int hour) {
  while (hour < 0) hour += 24;
  while (hour > 23) hour -= 24;
  return hour;
}

static int wrapMinute5(int minute) {
  while (minute < 0) minute += 60;
  while (minute > 59) minute -= 60;
  return (minute / 5) * 5;
}

static int clampDurationSeconds(int seconds) {
  if (seconds < PUMP_RUNTIME_MIN_SECONDS) return PUMP_RUNTIME_MIN_SECONDS;
  if (seconds > PUMP_RUNTIME_MAX_SECONDS) return PUMP_RUNTIME_MAX_SECONDS;
  return seconds;
}

static int getMinuteOfDay() {
  return (getHour() * 60) + getMinute();
}

static unsigned long clampPumpRuntime(unsigned long runtimeMs) {
  const unsigned long maxRuntimeMs = static_cast<unsigned long>(PUMP_RUNTIME_MAX_SECONDS) * 1000UL;
  if (runtimeMs == 0 || runtimeMs > maxRuntimeMs) return maxRuntimeMs;
  return runtimeMs;
}

static void refreshAutoRunCount() {
  int currentDateKey = getDateKey();
  if (currentDateKey < 0 || autoRunCountDateKey == currentDateKey) return;

  autoRunCountDateKey = currentDateKey;
  autoRunCount = 0;
}

static bool autoRunAlreadyStartedThisMinute() {
  int currentDateKey = getDateKey();
  return currentDateKey >= 0 && lastRunDateKey == currentDateKey && lastRunMinuteOfDay == getMinuteOfDay();
}

static bool canStartAutoPump() {
  refreshAutoRunCount();

  if (restMode_isEnabled()) return false;
  if (!isTimeSynced()) return false;
  if (growMode_isHarvest()) return false;
  if (pumpScheduler_isStartupLocked()) return false;
  if (getDateKey() < 0) return false;
  if (autoRunAlreadyStartedThisMinute()) return false;
  return true;
}

static void saveLastAutoRun() {
  if (!pumpSettingsReady || lastRunDateKey < 0) return;

  pumpSettings.putInt("lastDate", lastRunDateKey);
  pumpSettings.putInt("lastDay", lastRunDay);
  pumpSettings.putInt("lastMin", lastRunMinuteOfDay);
  pumpSettings.putInt("countDate", autoRunCountDateKey);
  pumpSettings.putInt("runCount", autoRunCount);
  Serial.println("Pump auto run saved");
}

static void loadLastAutoRun() {
  pumpSettingsReady = pumpSettings.begin("vgrow-pump", false);

  if (!pumpSettingsReady) {
    Serial.println("Pump settings unavailable");
    return;
  }

  lastRunDateKey = pumpSettings.getInt("lastDate", -1);
  lastRunDay = pumpSettings.getInt("lastDay", -1);
  pumpRunHour = wrapHour(pumpSettings.getInt("runH", PUMP_RUN_HOUR));
  pumpRunMinute = wrapMinute5(pumpSettings.getInt("runM", PUMP_RUN_MINUTE));
  lastRunMinuteOfDay = pumpSettings.getInt("lastMin", lastRunDateKey >= 0 ? (pumpRunHour * 60 + pumpRunMinute) : -1);
  autoRunCountDateKey = pumpSettings.getInt("countDate", lastRunDateKey);
  autoRunCount = pumpSettings.getInt("runCount", lastRunDateKey == autoRunCountDateKey ? 1 : 0);
  pumpRunDurationSeconds = clampDurationSeconds(pumpSettings.getInt("durS", PUMP_RUNTIME_MS / 1000UL));
  Serial.println("Pump settings loaded");
}

static void pumpScheduler_start(PumpMode mode, unsigned long runtimeMs) {
  if (pumpRunning) return;

  runtimeMs = clampPumpRuntime(runtimeMs);
  pump_on();
  pumpRunning = true;
  pumpStartTime = millis();
  pumpRuntimeMs = runtimeMs;
  pumpMode = mode;

  if (mode == PUMP_MODE_AUTO) {
    refreshAutoRunCount();
    lastRunDateKey = getDateKey();
    lastRunDay = getYearDay();
    lastRunMinuteOfDay = getMinuteOfDay();
    autoRunCountDateKey = lastRunDateKey;
    autoRunCount++;
    saveLastAutoRun();
  }

  Serial.print("PUMP START ");
  Serial.println(pumpScheduler_getModeName());
}

static void pumpScheduler_stop() {
  if (!pumpRunning) return;

  pump_off();
  pumpRunning = false;
  pumpRuntimeMs = 0;
  pumpMode = PUMP_MODE_IDLE;

  Serial.println("PUMP STOP");
}

void pumpScheduler_begin() {
  pump_begin();
  loadLastAutoRun();
}

void pumpScheduler_loop() {
  if (restMode_isEnabled()) {
    pumpScheduler_stop();
    return;
  }

  int currentDateKey = getDateKey();

  bool isTime =
    (autoScheduleEnabled &&
     isTimeSynced() &&
     !growMode_isHarvest() &&
     currentDateKey >= 0 &&
     getHour() == pumpRunHour &&
     getMinute() == pumpRunMinute);

  if (isTime && !pumpRunning && canStartAutoPump()) {
    pumpScheduler_start(PUMP_MODE_AUTO, static_cast<unsigned long>(pumpRunDurationSeconds) * 1000UL);
  }

  if (pumpRunning && millis() - pumpStartTime >= pumpRuntimeMs) {
    pumpScheduler_stop();
  }
}

void pumpScheduler_manualStart() {
  if (restMode_isEnabled()) return;
  pumpScheduler_start(PUMP_MODE_TEST, PUMP_TEST_RUNTIME_MS);
}

void pumpScheduler_manualStop() {
  pumpScheduler_stop();
}

void pumpScheduler_setAutoScheduleEnabled(bool enabled) {
  autoScheduleEnabled = enabled;
}

bool pumpScheduler_startAutoRunSeconds(int seconds) {
  if (seconds < PUMP_RUNTIME_MIN_SECONDS || seconds > PUMP_RUNTIME_MAX_SECONDS) return false;
  if (pumpRunning || !canStartAutoPump()) return false;
  pumpScheduler_start(PUMP_MODE_AUTO, static_cast<unsigned long>(seconds) * 1000UL);
  return true;
}

bool pumpScheduler_isRunning() {
  return pumpRunning;
}

int pumpScheduler_getLastRunDay() {
  return lastRunDay;
}

int pumpScheduler_getLastRunDateKey() {
  return lastRunDateKey;
}

bool pumpScheduler_wasRunToday() {
  refreshAutoRunCount();
  return autoRunCount > 0;
}

PumpMode pumpScheduler_getMode() {
  return pumpMode;
}

const char* pumpScheduler_getModeName() {
  switch (pumpMode) {
    case PUMP_MODE_AUTO: return "AUTO";
    case PUMP_MODE_TEST: return "TEST";
    case PUMP_MODE_IDLE:
    default: return "IDLE";
  }
}

const char* pumpScheduler_getReasonName() {
  if (restMode_isEnabled()) return "REST OFF";
  if (pumpRunning && pumpMode == PUMP_MODE_AUTO) return "RUN AUTO";
  if (pumpRunning && pumpMode == PUMP_MODE_TEST) return "RUN TEST";
  if (growMode_isHarvest()) return "HARVEST OFF";
  if (!isTimeSynced()) return "TIME WAIT";
  if (pumpScheduler_isStartupLocked()) return "BOOT LOCK";
  refreshAutoRunCount();
  if (autoRunAlreadyStartedThisMinute()) return "EVENT DONE";
  return "WAIT EVENT";
}

unsigned long pumpScheduler_getRemainingSeconds() {
  if (!pumpRunning) return 0;

  unsigned long elapsed = millis() - pumpStartTime;
  if (elapsed >= pumpRuntimeMs) return 0;

  return ((pumpRuntimeMs - elapsed) + 999UL) / 1000UL;
}

int pumpScheduler_getRunHour() {
  return pumpRunHour;
}

int pumpScheduler_getRunMinute() {
  return pumpRunMinute;
}

int pumpScheduler_getRunDurationSeconds() {
  return pumpRunDurationSeconds;
}

int pumpScheduler_getMaxRunDurationSeconds() {
  return PUMP_RUNTIME_MAX_SECONDS;
}

int pumpScheduler_getRunsToday() {
  refreshAutoRunCount();
  return autoRunCount;
}

bool pumpScheduler_isStartupLocked() {
  return millis() < PUMP_STARTUP_LOCK_MS;
}

const char* pumpScheduler_getSettingName(int index) {
  switch (index) {
    case PUMP_SETTING_TEST: return "Test";
    case PUMP_SETTING_HOUR: return "Hour";
    case PUMP_SETTING_MINUTE: return "Minute";
    case PUMP_SETTING_DURATION: return "Duration";
    default: return "Pump";
  }
}

bool pumpScheduler_settingsReady() {
  return pumpSettingsReady;
}

bool pumpScheduler_setSchedule(int hour, int minute) {
  if (!pumpSettingsReady) return false;
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || minute % 5 != 0) return false;

  pumpRunHour = hour;
  pumpRunMinute = minute;
  pumpSettings.putInt("runH", pumpRunHour);
  pumpSettings.putInt("runM", pumpRunMinute);
  pumpSettingsDirty = false;
  Serial.println("Pump schedule saved");
  return true;
}

bool pumpScheduler_setRunDurationSeconds(int seconds) {
  if (!pumpSettingsReady) return false;
  if (seconds < PUMP_RUNTIME_MIN_SECONDS || seconds > PUMP_RUNTIME_MAX_SECONDS) return false;

  pumpRunDurationSeconds = seconds;
  pumpSettings.putInt("durS", pumpRunDurationSeconds);
  pumpSettingsDirty = false;
  Serial.println("Pump duration saved");
  return true;
}

void pumpScheduler_adjustSetting(int index, int delta) {
  if (delta == 0) return;

  if (index == PUMP_SETTING_HOUR) {
    pumpRunHour = wrapHour(pumpRunHour + delta);
    pumpSettingsDirty = true;
  }

  if (index == PUMP_SETTING_MINUTE) {
    pumpRunMinute = wrapMinute5(pumpRunMinute + (delta * 5));
    pumpSettingsDirty = true;
  }

  if (index == PUMP_SETTING_DURATION) {
    pumpRunDurationSeconds = clampDurationSeconds(pumpRunDurationSeconds + delta);
    pumpSettingsDirty = true;
  }
}

void pumpScheduler_saveSettings() {
  if (!pumpSettingsReady || !pumpSettingsDirty) return;

  pumpSettings.putInt("runH", pumpRunHour);
  pumpSettings.putInt("runM", pumpRunMinute);
  pumpSettings.putInt("durS", pumpRunDurationSeconds);
  pumpSettingsDirty = false;
  Serial.println("Pump settings saved");
}
