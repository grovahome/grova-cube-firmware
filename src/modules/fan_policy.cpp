#include <Arduino.h>
#include <Preferences.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "modules/fan_control.h"
#include "modules/fan_interval.h"
#include "modules/fan_policy.h"
#include "modules/fan_schedule.h"
#include "modules/signal_registry.h"

static constexpr uint32_t FAN_POLICY_SCHEMA_VERSION = 1;
static constexpr unsigned long MAX_POLICY_DURATION_MS = 7UL * 24UL * 60UL * 60UL * 1000UL;

struct PersistedFanRule {
  uint8_t source;
  uint8_t enabled;
  uint8_t direction;
  uint8_t targetSource;
  float fixedTarget;
  float leadBeforeTarget;
  float fullLoadBeyondTarget;
  float hysteresis;
  uint32_t maxSignalAgeMs;
  uint8_t missingSignalBehavior;
  uint8_t curve;
};

struct PersistedFanSchedule {
  uint8_t enabled;
  int16_t startMinuteOfDay;
  int16_t endMinuteOfDay;
  int16_t percent;
};

struct PersistedFanPolicy {
  uint32_t schemaVersion;
  uint32_t structSize;
  uint8_t defaultMode;
  int16_t manualPercent;
  int16_t basePercent;
  uint32_t minimumRunMs;
  uint8_t ruleCount;
  PersistedFanRule rules[FAN_MAX_RULES_PER_CHANNEL];
  int16_t minimumPercent;
  int16_t maximumPercent;
  int16_t startupBoostPercent;
  uint32_t startupBoostMs;
  int16_t rampUpPwmStep;
  int16_t rampDownPwmStep;
  uint32_t rampIntervalMs;
  int16_t stallMinimumCheckPercent;
  int16_t stallMinimumRpm;
  uint32_t stallFaultDelayMs;
  uint8_t intervalEnabled;
  uint32_t intervalPeriodMs;
  uint32_t intervalRunMs;
  uint32_t intervalStartDelayMs;
  int16_t intervalPercent;
  PersistedFanSchedule schedules[FAN_MAX_SCHEDULES_PER_CHANNEL];
};

static Preferences settings;
static bool settingsReady = false;
static bool defaultsCaptured = false;
static FanDeviceConfig defaultDevices[FAN_CHANNEL_COUNT];
static FanAutomationConfig defaultAutomation[FAN_CHANNEL_COUNT];
static FanIntervalConfig defaultIntervals[FAN_CHANNEL_COUNT];
static FanScheduleConfig defaultSchedules[FAN_CHANNEL_COUNT][FAN_MAX_SCHEDULES_PER_CHANNEL];

static int fanIndex(int fan) {
  return fan >= 1 && fan <= FAN_CHANNEL_COUNT ? fan - 1 : 0;
}

static void response(String& json, bool ok, const char* message) {
  json = "{\"ok\":";
  json += ok ? "true" : "false";
  json += ",\"message\":\"";
  json += message;
  json += "\"}";
}

static void normalizeToken(char* value) {
  for (size_t i = 0; value[i] != '\0'; i++) {
    if (value[i] == '-' || value[i] == ' ') value[i] = '_';
    value[i] = toupper(static_cast<unsigned char>(value[i]));
  }
}

static int keyPosition(const String& body, const char* key) {
  String needle = "\"";
  needle += key;
  needle += "\"";
  return body.indexOf(needle);
}

static bool valuePosition(const String& body, const char* key, int& valueStart) {
  const int keyPos = keyPosition(body, key);
  if (keyPos < 0) return false;
  const int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return false;
  valueStart = colonPos + 1;
  while (valueStart < static_cast<int>(body.length()) && isspace(body[valueStart])) valueStart++;
  return valueStart < static_cast<int>(body.length());
}

static bool extractString(const String& body, const char* key, char* output, size_t outputSize) {
  if (outputSize == 0) return false;
  int start = 0;
  if (!valuePosition(body, key, start) || body[start] != '"') return false;
  const int end = body.indexOf('"', start + 1);
  if (end < 0) return false;
  const size_t length = min(static_cast<size_t>(end - start - 1), outputSize - 1);
  body.substring(start + 1, start + 1 + length).toCharArray(output, outputSize);
  return true;
}

static bool extractFloat(const String& body, const char* key, float& value) {
  int start = 0;
  if (!valuePosition(body, key, start)) return false;
  int end = start;
  while (end < static_cast<int>(body.length()) &&
         (isdigit(body[end]) || body[end] == '-' || body[end] == '.')) end++;
  if (end == start) return false;
  value = body.substring(start, end).toFloat();
  return true;
}

static bool extractInt(const String& body, const char* key, int& value) {
  float parsed = 0.0F;
  if (!extractFloat(body, key, parsed)) return false;
  value = lroundf(parsed);
  return true;
}

static bool extractUnsignedLong(const String& body, const char* key, unsigned long& value) {
  int start = 0;
  if (!valuePosition(body, key, start)) return false;
  int end = start;
  while (end < static_cast<int>(body.length()) && isdigit(body[end])) end++;
  if (end == start) return false;
  value = strtoul(body.substring(start, end).c_str(), nullptr, 10);
  return true;
}

static bool extractBool(const String& body, const char* key, bool& value) {
  int start = 0;
  if (!valuePosition(body, key, start)) return false;
  if (body.startsWith("true", start)) {
    value = true;
    return true;
  }
  if (body.startsWith("false", start)) {
    value = false;
    return true;
  }
  char token[12];
  if (!extractString(body, key, token, sizeof(token))) return false;
  normalizeToken(token);
  if (strcmp(token, "TRUE") == 0 || strcmp(token, "ON") == 0) {
    value = true;
    return true;
  }
  if (strcmp(token, "FALSE") == 0 || strcmp(token, "OFF") == 0) {
    value = false;
    return true;
  }
  return false;
}

static void captureDefaults() {
  if (defaultsCaptured) return;
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    const int index = fanIndex(fan);
    defaultDevices[index] = fanControl_getDeviceConfig(fan);
    defaultAutomation[index] = fanControl_getAutomationConfig(fan);
    defaultIntervals[index] = fanInterval_getConfig(fan);
    for (int schedule = 0; schedule < FAN_MAX_SCHEDULES_PER_CHANNEL; schedule++) {
      defaultSchedules[index][schedule] = fanSchedule_getConfig(fan, schedule);
    }
  }
  defaultsCaptured = true;
}

static void normalize(
  FanDeviceConfig& device,
  FanAutomationConfig& automation,
  FanIntervalConfig& interval,
  FanScheduleConfig schedules[FAN_MAX_SCHEDULES_PER_CHANNEL]
) {
  automation.defaultMode = static_cast<FanOperatingMode>(
    constrain(static_cast<int>(automation.defaultMode), FAN_MODE_OFF, FAN_MODE_AUTOMATIC));
  automation.manualPercent = constrain(automation.manualPercent, 0, 100);
  automation.basePercent = constrain(automation.basePercent, 0, 100);
  automation.minimumRunMs = constrain(automation.minimumRunMs, 0UL, MAX_POLICY_DURATION_MS);
  automation.ruleCount = constrain(automation.ruleCount, 0, FAN_MAX_RULES_PER_CHANNEL);

  for (int i = 0; i < FAN_MAX_RULES_PER_CHANNEL; i++) {
    FanSourceRuleConfig& rule = automation.rules[i];
    rule.source = static_cast<SignalId>(
      constrain(static_cast<int>(rule.source), 0, SIGNAL_COUNT - 1));
    rule.direction = static_cast<FanRuleDirection>(
      constrain(static_cast<int>(rule.direction), FAN_RULE_ABOVE, FAN_RULE_BELOW));
    rule.targetSource = static_cast<FanTargetSource>(
      constrain(static_cast<int>(rule.targetSource), FAN_TARGET_FIXED, FAN_TARGET_CLIMATE_HUMIDITY));
    rule.fixedTarget = constrain(rule.fixedTarget, -1000.0F, 10000.0F);
    rule.leadBeforeTarget = constrain(rule.leadBeforeTarget, 0.0F, 10000.0F);
    rule.fullLoadBeyondTarget = constrain(rule.fullLoadBeyondTarget, 0.1F, 10000.0F);
    rule.hysteresis = constrain(rule.hysteresis, 0.0F, 10000.0F);
    rule.maxSignalAgeMs = constrain(rule.maxSignalAgeMs, 1000UL, 60UL * 60UL * 1000UL);
    rule.missingSignalBehavior = static_cast<FanMissingSignalBehavior>(constrain(
      static_cast<int>(rule.missingSignalBehavior), FAN_SIGNAL_IGNORE, FAN_SIGNAL_SAFE_OUTPUT));
    rule.curve = static_cast<FanCurveStyle>(
      constrain(static_cast<int>(rule.curve), FAN_CURVE_GENTLE, FAN_CURVE_AGGRESSIVE));
  }

  device.minimumPercent = constrain(device.minimumPercent, 0, 100);
  device.maximumPercent = constrain(device.maximumPercent, max(1, device.minimumPercent), 100);
  device.startupBoostPercent = constrain(device.startupBoostPercent, 0, 100);
  device.startupBoostMs = constrain(device.startupBoostMs, 0UL, 60000UL);
  device.rampUpPwmStep = constrain(device.rampUpPwmStep, 1, 255);
  device.rampDownPwmStep = constrain(device.rampDownPwmStep, 1, 255);
  device.rampIntervalMs = constrain(device.rampIntervalMs, 10UL, 10000UL);
  device.stall.minimumCheckPercent = constrain(device.stall.minimumCheckPercent, 0, 100);
  device.stall.minimumRpm = constrain(device.stall.minimumRpm, 0, 20000);
  device.stall.faultDelayMs = constrain(device.stall.faultDelayMs, 1000UL, 10UL * 60UL * 1000UL);

  interval.periodMs = constrain(interval.periodMs, 1000UL, MAX_POLICY_DURATION_MS);
  interval.runMs = constrain(interval.runMs, 1000UL, interval.periodMs);
  interval.startDelayMs = constrain(interval.startDelayMs, 0UL, MAX_POLICY_DURATION_MS);
  interval.percent = constrain(interval.percent, 0, 100);

  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    schedules[i].startMinuteOfDay = constrain(schedules[i].startMinuteOfDay, 0, 1439);
    schedules[i].endMinuteOfDay = constrain(schedules[i].endMinuteOfDay, 0, 1439);
    schedules[i].percent = constrain(schedules[i].percent, 0, 100);
  }
}

static PersistedFanPolicy serialize(int fan) {
  PersistedFanPolicy stored{};
  const FanDeviceConfig& device = fanControl_getDeviceConfig(fan);
  const FanAutomationConfig& automation = fanControl_getAutomationConfig(fan);
  const FanIntervalConfig& interval = fanInterval_getConfig(fan);
  stored.schemaVersion = FAN_POLICY_SCHEMA_VERSION;
  stored.structSize = sizeof(PersistedFanPolicy);
  stored.defaultMode = automation.defaultMode;
  stored.manualPercent = automation.manualPercent;
  stored.basePercent = automation.basePercent;
  stored.minimumRunMs = automation.minimumRunMs;
  stored.ruleCount = automation.ruleCount;
  for (int i = 0; i < FAN_MAX_RULES_PER_CHANNEL; i++) {
    const FanSourceRuleConfig& source = automation.rules[i];
    PersistedFanRule& target = stored.rules[i];
    target.source = source.source;
    target.enabled = source.enabled;
    target.direction = source.direction;
    target.targetSource = source.targetSource;
    target.fixedTarget = source.fixedTarget;
    target.leadBeforeTarget = source.leadBeforeTarget;
    target.fullLoadBeyondTarget = source.fullLoadBeyondTarget;
    target.hysteresis = source.hysteresis;
    target.maxSignalAgeMs = source.maxSignalAgeMs;
    target.missingSignalBehavior = source.missingSignalBehavior;
    target.curve = source.curve;
  }
  stored.minimumPercent = device.minimumPercent;
  stored.maximumPercent = device.maximumPercent;
  stored.startupBoostPercent = device.startupBoostPercent;
  stored.startupBoostMs = device.startupBoostMs;
  stored.rampUpPwmStep = device.rampUpPwmStep;
  stored.rampDownPwmStep = device.rampDownPwmStep;
  stored.rampIntervalMs = device.rampIntervalMs;
  stored.stallMinimumCheckPercent = device.stall.minimumCheckPercent;
  stored.stallMinimumRpm = device.stall.minimumRpm;
  stored.stallFaultDelayMs = device.stall.faultDelayMs;
  stored.intervalEnabled = interval.enabled;
  stored.intervalPeriodMs = interval.periodMs;
  stored.intervalRunMs = interval.runMs;
  stored.intervalStartDelayMs = interval.startDelayMs;
  stored.intervalPercent = interval.percent;
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    const FanScheduleConfig& source = fanSchedule_getConfig(fan, i);
    stored.schedules[i] = {
      static_cast<uint8_t>(source.enabled),
      static_cast<int16_t>(source.startMinuteOfDay),
      static_cast<int16_t>(source.endMinuteOfDay),
      static_cast<int16_t>(source.percent)
    };
  }
  return stored;
}

static void applyStored(int fan, const PersistedFanPolicy& stored) {
  FanDeviceConfig device = fanControl_getDeviceConfig(fan);
  FanAutomationConfig automation = fanControl_getAutomationConfig(fan);
  FanIntervalConfig interval = fanInterval_getConfig(fan);
  FanScheduleConfig schedules[FAN_MAX_SCHEDULES_PER_CHANNEL];
  automation.defaultMode = static_cast<FanOperatingMode>(stored.defaultMode);
  automation.manualPercent = stored.manualPercent;
  automation.basePercent = stored.basePercent;
  automation.minimumRunMs = stored.minimumRunMs;
  automation.ruleCount = stored.ruleCount;
  for (int i = 0; i < FAN_MAX_RULES_PER_CHANNEL; i++) {
    const PersistedFanRule& source = stored.rules[i];
    FanSourceRuleConfig& target = automation.rules[i];
    target.source = static_cast<SignalId>(source.source);
    target.enabled = source.enabled;
    target.direction = static_cast<FanRuleDirection>(source.direction);
    target.targetSource = static_cast<FanTargetSource>(source.targetSource);
    target.fixedTarget = source.fixedTarget;
    target.leadBeforeTarget = source.leadBeforeTarget;
    target.fullLoadBeyondTarget = source.fullLoadBeyondTarget;
    target.hysteresis = source.hysteresis;
    target.maxSignalAgeMs = source.maxSignalAgeMs;
    target.missingSignalBehavior = static_cast<FanMissingSignalBehavior>(source.missingSignalBehavior);
    target.curve = static_cast<FanCurveStyle>(source.curve);
  }
  device.minimumPercent = stored.minimumPercent;
  device.maximumPercent = stored.maximumPercent;
  device.startupBoostPercent = stored.startupBoostPercent;
  device.startupBoostMs = stored.startupBoostMs;
  device.rampUpPwmStep = stored.rampUpPwmStep;
  device.rampDownPwmStep = stored.rampDownPwmStep;
  device.rampIntervalMs = stored.rampIntervalMs;
  device.stall.minimumCheckPercent = stored.stallMinimumCheckPercent;
  device.stall.minimumRpm = stored.stallMinimumRpm;
  device.stall.faultDelayMs = stored.stallFaultDelayMs;
  interval.enabled = stored.intervalEnabled;
  interval.periodMs = stored.intervalPeriodMs;
  interval.runMs = stored.intervalRunMs;
  interval.startDelayMs = stored.intervalStartDelayMs;
  interval.percent = stored.intervalPercent;
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    schedules[i] = {
      stored.schedules[i].enabled != 0,
      stored.schedules[i].startMinuteOfDay,
      stored.schedules[i].endMinuteOfDay,
      stored.schedules[i].percent
    };
  }
  normalize(device, automation, interval, schedules);
  fanControl_setDeviceConfig(fan, device);
  fanControl_setAutomationConfig(fan, automation);
  fanInterval_setConfig(fan, interval);
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    fanSchedule_setConfig(fan, i, schedules[i]);
  }
}

static bool saveFan(int fan) {
  if (!settingsReady || fan < 1 || fan > FAN_CHANNEL_COUNT) return false;
  const PersistedFanPolicy stored = serialize(fan);
  char key[8];
  snprintf(key, sizeof(key), "fan%d", fan);
  return settings.putBytes(key, &stored, sizeof(stored)) == sizeof(stored);
}

static bool loadFan(int fan) {
  char key[8];
  snprintf(key, sizeof(key), "fan%d", fan);
  if (settings.getBytesLength(key) != sizeof(PersistedFanPolicy)) return false;
  PersistedFanPolicy stored{};
  if (settings.getBytes(key, &stored, sizeof(stored)) != sizeof(stored)) return false;
  if (stored.schemaVersion != FAN_POLICY_SCHEMA_VERSION ||
      stored.structSize != sizeof(PersistedFanPolicy)) return false;
  applyStored(fan, stored);
  return true;
}

void fanPolicy_begin() {
  captureDefaults();
  settingsReady = settings.begin("grova-fan", false);
  if (!settingsReady) {
    Serial.println("Fan policy settings unavailable");
    return;
  }
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    if (!loadFan(fan)) saveFan(fan);
  }
  Serial.println("Fan policies loaded");
}

bool fanPolicy_settingsReady() {
  return settingsReady;
}

uint32_t fanPolicy_schemaVersion() {
  return FAN_POLICY_SCHEMA_VERSION;
}

int fanPolicy_extractFan(const String& body) {
  int fan = 0;
  return extractInt(body, "fan", fan) ? fan : 0;
}

bool fanPolicy_setOperatingMode(int fan, FanOperatingMode mode, int manualPercent) {
  if (fan < 1 || fan > FAN_CHANNEL_COUNT) return false;
  FanAutomationConfig automation = fanControl_getAutomationConfig(fan);
  automation.defaultMode = mode;
  if (mode == FAN_MODE_MANUAL) automation.manualPercent = constrain(manualPercent, 0, 100);
  fanControl_setAutomationConfig(fan, automation);
  return !settingsReady || saveFan(fan);
}

bool fanPolicy_reset(int fan) {
  if (fan < 1 || fan > FAN_CHANNEL_COUNT || !defaultsCaptured) return false;
  const int index = fanIndex(fan);
  fanControl_setDeviceConfig(fan, defaultDevices[index]);
  fanControl_setAutomationConfig(fan, defaultAutomation[index]);
  fanInterval_setConfig(fan, defaultIntervals[index]);
  for (int schedule = 0; schedule < FAN_MAX_SCHEDULES_PER_CHANNEL; schedule++) {
    fanSchedule_setConfig(fan, schedule, defaultSchedules[index][schedule]);
  }
  return !settingsReady || saveFan(fan);
}

static bool parseMode(const char* value, FanOperatingMode& mode) {
  char token[20];
  strlcpy(token, value, sizeof(token));
  normalizeToken(token);
  if (strcmp(token, "OFF") == 0) mode = FAN_MODE_OFF;
  else if (strcmp(token, "MANUAL") == 0) mode = FAN_MODE_MANUAL;
  else if (strcmp(token, "AUTO") == 0 || strcmp(token, "AUTOMATIC") == 0) mode = FAN_MODE_AUTOMATIC;
  else return false;
  return true;
}

bool fanPolicy_applyJson(const String& body, String& responseJson) {
  if (!settingsReady) {
    response(responseJson, false, "fan policy settings unavailable");
    return false;
  }
  int fan = 0;
  if (!extractInt(body, "fan", fan) || fan < 1 || fan > FAN_CHANNEL_COUNT) {
    response(responseJson, false, "invalid fan");
    return false;
  }
  char section[20];
  if (!extractString(body, "section", section, sizeof(section))) {
    response(responseJson, false, "missing fan policy section");
    return false;
  }
  normalizeToken(section);

  if (strcmp(section, "RESET") == 0) {
    const bool ok = fanPolicy_reset(fan);
    response(responseJson, ok, ok ? "fan policy reset" : "fan policy reset failed");
    return ok;
  }

  FanDeviceConfig device = fanControl_getDeviceConfig(fan);
  FanAutomationConfig automation = fanControl_getAutomationConfig(fan);
  FanIntervalConfig interval = fanInterval_getConfig(fan);
  FanScheduleConfig schedules[FAN_MAX_SCHEDULES_PER_CHANNEL];
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    schedules[i] = fanSchedule_getConfig(fan, i);
  }

  bool touched = false;
  int intValue = 0;
  unsigned long unsignedValue = 0;
  float floatValue = 0.0F;
  bool boolValue = false;
  char token[48];

  if (strcmp(section, "AUTOMATION") == 0) {
    if (extractString(body, "default_mode", token, sizeof(token))) {
      if (!parseMode(token, automation.defaultMode)) {
        response(responseJson, false, "invalid default mode");
        return false;
      }
      touched = true;
    }
    if (extractInt(body, "manual_percent", intValue)) {
      automation.manualPercent = intValue;
      touched = true;
    }
    if (extractInt(body, "base_percent", intValue)) {
      automation.basePercent = intValue;
      touched = true;
    }
    if (extractUnsignedLong(body, "minimum_run_ms", unsignedValue)) {
      automation.minimumRunMs = unsignedValue;
      touched = true;
    }
  } else if (strcmp(section, "RULE") == 0) {
    int ruleIndex = -1;
    if (!extractInt(body, "rule_index", ruleIndex) || ruleIndex < 0 ||
        ruleIndex >= FAN_MAX_RULES_PER_CHANNEL) {
      response(responseJson, false, "invalid rule index");
      return false;
    }
    if (ruleIndex >= automation.ruleCount) automation.ruleCount = ruleIndex + 1;
    FanSourceRuleConfig& rule = automation.rules[ruleIndex];
    if (extractBool(body, "enabled", boolValue)) { rule.enabled = boolValue; touched = true; }
    if (extractString(body, "signal", token, sizeof(token))) {
      SignalId signal;
      if (!signalRegistry_idFromName(token, signal)) {
        response(responseJson, false, "invalid signal");
        return false;
      }
      rule.source = signal;
      touched = true;
    }
    if (extractString(body, "direction", token, sizeof(token))) {
      normalizeToken(token);
      if (strcmp(token, "ABOVE") == 0) rule.direction = FAN_RULE_ABOVE;
      else if (strcmp(token, "BELOW") == 0) rule.direction = FAN_RULE_BELOW;
      else { response(responseJson, false, "invalid direction"); return false; }
      touched = true;
    }
    if (extractString(body, "target_source", token, sizeof(token))) {
      normalizeToken(token);
      if (strcmp(token, "FIXED") == 0) rule.targetSource = FAN_TARGET_FIXED;
      else if (strcmp(token, "CLIMATE_TEMPERATURE") == 0) rule.targetSource = FAN_TARGET_CLIMATE_TEMPERATURE;
      else if (strcmp(token, "CLIMATE_HUMIDITY") == 0) rule.targetSource = FAN_TARGET_CLIMATE_HUMIDITY;
      else { response(responseJson, false, "invalid target source"); return false; }
      touched = true;
    }
    if (extractFloat(body, "fixed_target", floatValue)) { rule.fixedTarget = floatValue; touched = true; }
    if (extractFloat(body, "lead_before_target", floatValue)) { rule.leadBeforeTarget = floatValue; touched = true; }
    if (extractFloat(body, "full_load_beyond_target", floatValue)) { rule.fullLoadBeyondTarget = floatValue; touched = true; }
    if (extractFloat(body, "hysteresis", floatValue)) { rule.hysteresis = floatValue; touched = true; }
    if (extractUnsignedLong(body, "max_signal_age_ms", unsignedValue)) { rule.maxSignalAgeMs = unsignedValue; touched = true; }
    if (extractString(body, "missing_signal", token, sizeof(token))) {
      normalizeToken(token);
      if (strcmp(token, "IGNORE") == 0) rule.missingSignalBehavior = FAN_SIGNAL_IGNORE;
      else if (strcmp(token, "SAFE_OUTPUT") == 0) rule.missingSignalBehavior = FAN_SIGNAL_SAFE_OUTPUT;
      else { response(responseJson, false, "invalid missing signal behavior"); return false; }
      touched = true;
    }
    if (extractString(body, "curve", token, sizeof(token))) {
      normalizeToken(token);
      if (strcmp(token, "GENTLE") == 0) rule.curve = FAN_CURVE_GENTLE;
      else if (strcmp(token, "NORMAL") == 0) rule.curve = FAN_CURVE_NORMAL;
      else if (strcmp(token, "AGGRESSIVE") == 0) rule.curve = FAN_CURVE_AGGRESSIVE;
      else { response(responseJson, false, "invalid curve"); return false; }
      touched = true;
    }
  } else if (strcmp(section, "INTERVAL") == 0) {
    if (extractBool(body, "enabled", boolValue)) { interval.enabled = boolValue; touched = true; }
    if (extractUnsignedLong(body, "period_ms", unsignedValue)) { interval.periodMs = unsignedValue; touched = true; }
    if (extractUnsignedLong(body, "run_ms", unsignedValue)) { interval.runMs = unsignedValue; touched = true; }
    if (extractUnsignedLong(body, "start_delay_ms", unsignedValue)) { interval.startDelayMs = unsignedValue; touched = true; }
    if (extractInt(body, "percent", intValue)) { interval.percent = intValue; touched = true; }
  } else if (strcmp(section, "SCHEDULE") == 0) {
    int scheduleIndex = -1;
    if (!extractInt(body, "schedule_index", scheduleIndex) || scheduleIndex < 0 ||
        scheduleIndex >= FAN_MAX_SCHEDULES_PER_CHANNEL) {
      response(responseJson, false, "invalid schedule index");
      return false;
    }
    FanScheduleConfig& schedule = schedules[scheduleIndex];
    if (extractBool(body, "enabled", boolValue)) { schedule.enabled = boolValue; touched = true; }
    if (extractInt(body, "start_minute", intValue)) { schedule.startMinuteOfDay = intValue; touched = true; }
    if (extractInt(body, "end_minute", intValue)) { schedule.endMinuteOfDay = intValue; touched = true; }
    if (extractInt(body, "percent", intValue)) { schedule.percent = intValue; touched = true; }
  } else if (strcmp(section, "DEVICE") == 0) {
    if (extractInt(body, "minimum_percent", intValue)) { device.minimumPercent = intValue; touched = true; }
    if (extractInt(body, "maximum_percent", intValue)) { device.maximumPercent = intValue; touched = true; }
    if (extractInt(body, "startup_boost_percent", intValue)) { device.startupBoostPercent = intValue; touched = true; }
    if (extractUnsignedLong(body, "startup_boost_ms", unsignedValue)) { device.startupBoostMs = unsignedValue; touched = true; }
    if (extractInt(body, "ramp_up_pwm_step", intValue)) { device.rampUpPwmStep = intValue; touched = true; }
    if (extractInt(body, "ramp_down_pwm_step", intValue)) { device.rampDownPwmStep = intValue; touched = true; }
    if (extractUnsignedLong(body, "ramp_interval_ms", unsignedValue)) { device.rampIntervalMs = unsignedValue; touched = true; }
    if (extractInt(body, "stall_minimum_check_percent", intValue)) { device.stall.minimumCheckPercent = intValue; touched = true; }
    if (extractInt(body, "stall_minimum_rpm", intValue)) { device.stall.minimumRpm = intValue; touched = true; }
    if (extractUnsignedLong(body, "stall_fault_delay_ms", unsignedValue)) { device.stall.faultDelayMs = unsignedValue; touched = true; }
  } else {
    response(responseJson, false, "invalid fan policy section");
    return false;
  }

  if (!touched) {
    response(responseJson, false, "no fan policy values supplied");
    return false;
  }

  normalize(device, automation, interval, schedules);
  fanControl_setDeviceConfig(fan, device);
  fanControl_setAutomationConfig(fan, automation);
  fanInterval_setConfig(fan, interval);
  for (int i = 0; i < FAN_MAX_SCHEDULES_PER_CHANNEL; i++) {
    fanSchedule_setConfig(fan, i, schedules[i]);
  }
  const bool ok = saveFan(fan);
  response(responseJson, ok, ok ? "fan policy saved" : "fan policy save failed");
  return ok;
}
