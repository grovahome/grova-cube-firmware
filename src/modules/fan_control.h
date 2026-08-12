#pragma once

#include <Arduino.h>

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
  FanSourceRuleConfig temperature;
  FanSourceRuleConfig humidity;
};

struct FanRuleState {
  bool temperatureActive = false;
  bool humidityActive = false;
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
  float temperature,
  float humidity,
  float temperatureTarget,
  float humidityTarget,
  FanRuleState& state
);
void fanControl_appendJson(String& json);
