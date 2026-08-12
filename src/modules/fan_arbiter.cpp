#include <Arduino.h>
#include "config.h"
#include "modules/fan_arbiter.h"

static int clampRunningPercent(int requested, const FanDeviceConfig& config) {
  if (requested <= 0) return 0;
  return constrain(requested, config.minimumPercent, config.maximumPercent);
}

FanArbiter::FanArbiter(
  const FanDeviceConfig& deviceConfig,
  const FanAutomationConfig& automationConfig
) : deviceConfig_(deviceConfig), automationConfig_(automationConfig) {}

const char* fanArbiter_priorityName(FanDecisionPriority priority) {
  if (priority == FAN_PRIORITY_DEVICE_FAULT) return "DEVICE_FAULT";
  if (priority == FAN_PRIORITY_REST) return "REST";
  if (priority == FAN_PRIORITY_SENSOR_SAFETY) return "SENSOR_SAFETY";
  if (priority == FAN_PRIORITY_MANUAL) return "MANUAL";
  if (priority == FAN_PRIORITY_AUTOMATIC) return "AUTOMATIC";
  return "IDLE";
}

void FanArbiter::reset() {
  automaticRunSince_ = 0;
}

FanArbiterResult FanArbiter::evaluate(const FanArbiterInput& input) {
  FanArbiterResult result;
  const unsigned long now = millis();

  if (input.deviceFault) {
    automaticRunSince_ = 0;
    result.priority = FAN_PRIORITY_DEVICE_FAULT;
    result.immediateStop = true;
    result.reason = "STALL OFF";
    return result;
  }

  if (input.restMode) {
    automaticRunSince_ = 0;
    result.priority = FAN_PRIORITY_REST;
    result.immediateStop = true;
    result.reason = "REST OFF";
    return result;
  }

  if (input.sensorFault && input.mode == FAN_MODE_AUTOMATIC) {
    automaticRunSince_ = 0;
    result.priority = FAN_PRIORITY_SENSOR_SAFETY;
    result.requestedPercent = clampRunningPercent(FAN_SENSOR_FAIL_PERCENT, deviceConfig_);
    result.reason = "SENSOR SAFE";
    return result;
  }

  if (input.mode == FAN_MODE_OFF) {
    automaticRunSince_ = 0;
    result.reason = "OFF";
    return result;
  }

  if (input.mode == FAN_MODE_MANUAL || input.legacyManualOverride) {
    automaticRunSince_ = 0;
    result.priority = FAN_PRIORITY_MANUAL;
    const int manualPercent = input.mode == FAN_MODE_MANUAL
      ? input.manualPercent
      : input.legacyManualPercent;
    result.requestedPercent = clampRunningPercent(manualPercent, deviceConfig_);
    result.reason = "MANUAL";
    return result;
  }

  result.priority = FAN_PRIORITY_AUTOMATIC;
  result.temperatureDemandPercent = input.automaticDemand.temperaturePercent;
  result.humidityDemandPercent = input.automaticDemand.humidityPercent;
  int requested = input.automaticDemand.percent;

  if (requested > 0) {
    if (automaticRunSince_ == 0) automaticRunSince_ = now;
    result.reason = input.automaticDemand.reason;
  } else if (automaticRunSince_ != 0 && now - automaticRunSince_ < automationConfig_.minimumRunMs) {
    requested = deviceConfig_.minimumPercent;
    result.reason = "MIN RUN";
  } else {
    automaticRunSince_ = 0;
    result.priority = FAN_PRIORITY_IDLE;
    result.reason = input.automaticDemand.reason;
  }

  result.requestedPercent = clampRunningPercent(requested, deviceConfig_);
  return result;
}
