#include <Arduino.h>
#include "modules/fan_interval.h"

// Code-only defaults. Both channels have the same interval capability and the
// producer remains disabled until a channel policy explicitly enables it.
static FanIntervalConfig FAN_INTERVAL_CONFIGS[2] = {
  {false, 10UL * 60UL * 1000UL, 2UL * 60UL * 1000UL, 10UL * 60UL * 1000UL, 30},
  {false, 10UL * 60UL * 1000UL, 2UL * 60UL * 1000UL, 10UL * 60UL * 1000UL, 30}
};

struct FanIntervalRuntime {
  bool initialized = false;
  bool started = false;
  unsigned long initializedAtMs = 0;
  unsigned long cycleStartedAtMs = 0;
};

static FanIntervalRuntime runtimes[2];

static int indexFor(int fan) {
  return fan == 2 ? 1 : 0;
}

const FanIntervalConfig& fanInterval_getConfig(int fan) {
  return FAN_INTERVAL_CONFIGS[indexFor(fan)];
}

void fanInterval_setConfig(int fan, const FanIntervalConfig& config) {
  if (fan < 1 || fan > 2) return;
  FAN_INTERVAL_CONFIGS[fan - 1] = config;
  fanInterval_resetRuntime(fan);
}

void fanInterval_begin() {
  runtimes[0] = FanIntervalRuntime{};
  runtimes[1] = FanIntervalRuntime{};
}

void fanInterval_resetRuntime(int fan) {
  if (fan < 1 || fan > 2) return;
  runtimes[fan - 1] = FanIntervalRuntime{};
}

FanIntervalDemand fanInterval_evaluate(int fan) {
  const FanIntervalConfig& config = fanInterval_getConfig(fan);
  FanIntervalDemand demand;
  if (!config.enabled || config.periodMs == 0 || config.runMs == 0 ||
      config.percent <= 0) {
    return demand;
  }

  const unsigned long now = millis();
  FanIntervalRuntime& runtime = runtimes[indexFor(fan)];
  if (!runtime.initialized) {
    runtime.initialized = true;
    runtime.initializedAtMs = now;
  }
  if (!runtime.started) {
    if (now - runtime.initializedAtMs < config.startDelayMs) return demand;
    runtime.started = true;
    runtime.cycleStartedAtMs = now;
  }

  demand.phaseMs = (now - runtime.cycleStartedAtMs) % config.periodMs;
  demand.active = config.runMs >= config.periodMs || demand.phaseMs < config.runMs;
  demand.percent = demand.active ? constrain(config.percent, 0, 100) : 0;
  return demand;
}

void fanInterval_appendJson(String& json, int fan) {
  const FanIntervalConfig& config = fanInterval_getConfig(fan);
  json += "\"interval\":{";
  json += "\"enabled\":";
  json += config.enabled ? "true" : "false";
  json += ",\"period_ms\":";
  json += config.periodMs;
  json += ",\"run_ms\":";
  json += config.runMs;
  json += ",\"start_delay_ms\":";
  json += config.startDelayMs;
  json += ",\"percent\":";
  json += config.percent;
  json += "}";
}
