#pragma once

void time_begin();
void time_loop();

int getHour();
int getMinute();
int getYearDay();
int getDateKey();
unsigned long getEpochSeconds();
unsigned long long getEpochMillis();
bool isTimeSynced();
bool time_settingsReady();

bool rtc_isEnabled();
bool rtc_isPresent();
bool rtc_hasValidTime();
bool rtc_wasUsedForBoot();
bool rtc_lastReadOk();
bool rtc_lastWriteOk();
const char* time_getSourceName();
bool rtc_setEnabled(bool enabled);
