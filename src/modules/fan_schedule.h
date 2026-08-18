#pragma once

#include <Arduino.h>

constexpr int FAN_MAX_SCHEDULES_PER_CHANNEL = 3;

struct FanScheduleConfig {
  bool enabled;
  int startMinuteOfDay;
  int endMinuteOfDay;
  int percent;
};

struct FanScheduleDemand {
  int percent = 0;
  bool active = false;
  bool timeAvailable = false;
  int winningScheduleIndex = -1;
  const char* reason = "SCHEDULE";
};

const FanScheduleConfig& fanSchedule_getConfig(int fan, int scheduleIndex);
void fanSchedule_setConfig(int fan, int scheduleIndex, const FanScheduleConfig& config);
int fanSchedule_getCount(int fan);
FanScheduleDemand fanSchedule_evaluate(int fan);
void fanSchedule_appendJson(String& json, int fan);
