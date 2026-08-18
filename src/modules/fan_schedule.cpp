#include <Arduino.h>
#include "modules/fan_schedule.h"
#include "modules/time_sync.h"

static FanScheduleConfig FAN_SCHEDULE_CONFIGS[2][FAN_MAX_SCHEDULES_PER_CHANNEL] = {
  {
    {false, 8 * 60, 20 * 60, 30},
    {false, 0, 0, 0},
    {false, 0, 0, 0}
  },
  {
    {false, 8 * 60, 20 * 60, 30},
    {false, 0, 0, 0},
    {false, 0, 0, 0}
  }
};

static int fanIndex(int fan) {
  return fan == 2 ? 1 : 0;
}

static int clampScheduleIndex(int scheduleIndex) {
  return constrain(scheduleIndex, 0, FAN_MAX_SCHEDULES_PER_CHANNEL - 1);
}

static bool isWithinWindow(int minuteOfDay, int startMinute, int endMinute) {
  if (startMinute == endMinute) return true;
  if (startMinute < endMinute) {
    return minuteOfDay >= startMinute && minuteOfDay < endMinute;
  }
  return minuteOfDay >= startMinute || minuteOfDay < endMinute;
}

const FanScheduleConfig& fanSchedule_getConfig(int fan, int scheduleIndex) {
  return FAN_SCHEDULE_CONFIGS[fanIndex(fan)][clampScheduleIndex(scheduleIndex)];
}

void fanSchedule_setConfig(int fan, int scheduleIndex, const FanScheduleConfig& config) {
  if (fan < 1 || fan > 2 || scheduleIndex < 0 ||
      scheduleIndex >= FAN_MAX_SCHEDULES_PER_CHANNEL) {
    return;
  }
  FAN_SCHEDULE_CONFIGS[fan - 1][scheduleIndex] = config;
}

int fanSchedule_getCount(int fan) {
  (void)fan;
  return FAN_MAX_SCHEDULES_PER_CHANNEL;
}

FanScheduleDemand fanSchedule_evaluate(int fan) {
  FanScheduleDemand demand;
  demand.timeAvailable = isTimeSynced();
  if (!demand.timeAvailable) return demand;

  const int minuteOfDay = getHour() * 60 + getMinute();
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    const FanScheduleConfig& config = fanSchedule_getConfig(fan, i);
    if (!config.enabled || config.percent <= 0) continue;
    if (!isWithinWindow(minuteOfDay, config.startMinuteOfDay, config.endMinuteOfDay)) continue;

    const int percent = constrain(config.percent, 0, 100);
    if (percent > demand.percent) {
      demand.percent = percent;
      demand.winningScheduleIndex = i;
    }
  }

  demand.active = demand.percent > 0;
  return demand;
}

void fanSchedule_appendJson(String& json, int fan) {
  json += "\"schedules\":[";
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    if (i > 0) json += ",";
    const FanScheduleConfig& config = fanSchedule_getConfig(fan, i);
    json += "{";
    json += "\"index\":";
    json += i;
    json += ",\"enabled\":";
    json += config.enabled ? "true" : "false";
    json += ",\"start_minute\":";
    json += config.startMinuteOfDay;
    json += ",\"end_minute\":";
    json += config.endMinuteOfDay;
    json += ",\"percent\":";
    json += config.percent;
    json += "}";
  }
  json += "]";
}
