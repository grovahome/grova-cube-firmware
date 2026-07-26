#pragma once

enum PumpMode {
  PUMP_MODE_IDLE,
  PUMP_MODE_AUTO,
  PUMP_MODE_TEST
};

enum PumpSetting {
  PUMP_SETTING_TEST,
  PUMP_SETTING_HOUR,
  PUMP_SETTING_MINUTE,
  PUMP_SETTING_DURATION,
  PUMP_SETTING_COUNT
};

void pumpScheduler_begin();
void pumpScheduler_loop();
void pumpScheduler_setAutoScheduleEnabled(bool enabled);

void pumpScheduler_manualStart();
void pumpScheduler_manualStop();
bool pumpScheduler_startAutoRunSeconds(int seconds);
bool pumpScheduler_isRunning();
int pumpScheduler_getLastRunDay();
int pumpScheduler_getLastRunDateKey();
bool pumpScheduler_wasRunToday();
PumpMode pumpScheduler_getMode();
const char* pumpScheduler_getModeName();
const char* pumpScheduler_getReasonName();
unsigned long pumpScheduler_getRemainingSeconds();
int pumpScheduler_getRunHour();
int pumpScheduler_getRunMinute();
int pumpScheduler_getRunDurationSeconds();
int pumpScheduler_getMaxRunDurationSeconds();
int pumpScheduler_getRunsToday();
int pumpScheduler_getMaxRunsPerDay();
int pumpScheduler_getMinIntervalHours();
bool pumpScheduler_isStartupLocked();
const char* pumpScheduler_getSettingName(int index);
bool pumpScheduler_settingsReady();
bool pumpScheduler_setSchedule(int hour, int minute);
bool pumpScheduler_setRunDurationSeconds(int seconds);
void pumpScheduler_adjustSetting(int index, int delta);
void pumpScheduler_saveSettings();
