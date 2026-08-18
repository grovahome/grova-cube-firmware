#pragma once

#include <Arduino.h>

struct FanIntervalConfig {
  bool enabled;
  unsigned long periodMs;
  unsigned long runMs;
  unsigned long startDelayMs;
  int percent;
};

struct FanIntervalDemand {
  int percent = 0;
  bool active = false;
  unsigned long phaseMs = 0;
  const char* reason = "INTERVAL";
};

const FanIntervalConfig& fanInterval_getConfig(int fan);
void fanInterval_setConfig(int fan, const FanIntervalConfig& config);
void fanInterval_begin();
void fanInterval_resetRuntime(int fan);
FanIntervalDemand fanInterval_evaluate(int fan);
void fanInterval_appendJson(String& json, int fan);
