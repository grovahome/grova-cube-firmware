#pragma once

#include <Arduino.h>
#include "modules/signal_registry.h"

constexpr int FAN_CHANNEL_COUNT = 2;
constexpr int FAN_MAX_RULES_PER_CHANNEL = 6;

enum FanOperatingMode {
  FAN_MODE_OFF = 0,
  FAN_MODE_MANUAL = 1,
  FAN_MODE_AUTOMATIC = 2
};

enum FanCurveStyle {
  FAN_CURVE_GENTLE = 0,
  FAN_CURVE_NORMAL = 1,
  FAN_CURVE_AGGRESSIVE = 2
};

enum FanRuleDirection {
  FAN_RULE_ABOVE = 0,
  FAN_RULE_BELOW = 1
};

enum FanTargetSource {
  FAN_TARGET_FIXED = 0,
  FAN_TARGET_CLIMATE_TEMPERATURE = 1,
  FAN_TARGET_CLIMATE_HUMIDITY = 2
};

enum FanMissingSignalBehavior {
  FAN_SIGNAL_IGNORE = 0,
  FAN_SIGNAL_SAFE_OUTPUT = 1
};

struct FanSourceRuleConfig {
  const char* id;
  SignalId source;
  bool enabled;
  FanRuleDirection direction;
  FanTargetSource targetSource;
  float fixedTarget;
  float leadBeforeTarget;
  float fullLoadBeyondTarget;
  float hysteresis;
  unsigned long maxSignalAgeMs;
  FanMissingSignalBehavior missingSignalBehavior;
  FanCurveStyle curve;
};

struct FanStallConfig {
  int minimumCheckPercent;
  int minimumRpm;
  unsigned long faultDelayMs;
};

struct FanDeviceConfig {
  int minimumPercent;
  int maximumPercent;
  int startupBoostPercent;
  unsigned long startupBoostMs;
  int rampUpPwmStep;
  int rampDownPwmStep;
  unsigned long rampIntervalMs;
  FanStallConfig stall;
};

struct FanAutomationConfig {
  FanOperatingMode defaultMode;
  int manualPercent;
  int basePercent;
  unsigned long minimumRunMs;
  int ruleCount;
  FanSourceRuleConfig rules[FAN_MAX_RULES_PER_CHANNEL];
};

struct FanRuleState {
  bool active[FAN_MAX_RULES_PER_CHANNEL] = {};
};

struct FanRuleDemand {
  int percent = 0;
  bool active = false;
  bool signalValid = false;
  SignalId source = SIGNAL_CLIMATE_TEMPERATURE_C;
  const char* ruleId = "NONE";
};

struct FanDemand {
  int percent = 0;
  int curvePercent = 0;
  int basePercent = 0;
  int intervalPercent = 0;
  int schedulePercent = 0;
  int temperaturePercent = 0;
  int humidityPercent = 0;
  int ruleCount = 0;
  int winningRuleIndex = -1;
  bool requiresHealthySignals = false;
  bool signalSafetyRequired = false;
  FanRuleDemand rules[FAN_MAX_RULES_PER_CHANNEL];
  const char* curveReason = "IDLE";
  const char* winningProducer = "NONE";
  const char* reason = "IDLE";
};

const FanDeviceConfig& fanControl_getDeviceConfig(int fan);
const FanAutomationConfig& fanControl_getAutomationConfig(int fan);
void fanControl_setDeviceConfig(int fan, const FanDeviceConfig& config);
void fanControl_setAutomationConfig(int fan, const FanAutomationConfig& config);
const char* fanControl_modeName(FanOperatingMode mode);
const char* fanControl_curveName(FanCurveStyle curve);
const char* fanControl_directionName(FanRuleDirection direction);
const char* fanControl_targetSourceName(FanTargetSource source);
const char* fanControl_missingSignalBehaviorName(FanMissingSignalBehavior behavior);
FanDemand fanControl_evaluateCurves(
  int fan,
  float temperatureTarget,
  float humidityTarget,
  FanRuleState& state
);
void fanControl_appendJson(String& json);
