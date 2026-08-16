#pragma once

#include <Arduino.h>
#include "modules/signal_registry.h"

constexpr int FAN_CHANNEL_COUNT = 2;
constexpr int FAN_RULE_SOURCE_COUNT = 2;

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

struct FanSourceRuleConfig {
  SignalId source;
  bool enabled;
  float leadBeforeTarget;
  float fullLoadAboveTarget;
  float hysteresis;
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
  FanSourceRuleConfig rules[FAN_RULE_SOURCE_COUNT];
};

struct FanRuleState {
  bool active[FAN_RULE_SOURCE_COUNT] = {false, false};
};

struct FanDemand {
  int percent = 0;
  int temperaturePercent = 0;
  int humidityPercent = 0;
  const char* reason = "IDLE";
};

const FanDeviceConfig& fanControl_getDeviceConfig(int fan);
const FanAutomationConfig& fanControl_getAutomationConfig(int fan);
const char* fanControl_modeName(FanOperatingMode mode);
const char* fanControl_curveName(FanCurveStyle curve);
FanDemand fanControl_evaluate(
  int fan,
  float temperatureTarget,
  float humidityTarget,
  FanRuleState& state
);
void fanControl_appendJson(String& json);
