#pragma once

#include "modules/fan_control.h"

enum FanDecisionPriority {
  FAN_PRIORITY_IDLE = 0,
  FAN_PRIORITY_AUTOMATIC = 10,
  FAN_PRIORITY_MANUAL = 20,
  FAN_PRIORITY_SENSOR_SAFETY = 30,
  FAN_PRIORITY_REST = 40,
  FAN_PRIORITY_DEVICE_FAULT = 50
};

struct FanArbiterInput {
  FanOperatingMode mode = FAN_MODE_OFF;
  int manualPercent = 0;
  bool legacyManualOverride = false;
  int legacyManualPercent = 0;
  bool restMode = false;
  bool sensorFault = false;
  bool deviceFault = false;
  FanDemand automaticDemand;
};

struct FanArbiterResult {
  int requestedPercent = 0;
  FanDemand automaticDemand;
  FanDecisionPriority priority = FAN_PRIORITY_IDLE;
  bool immediateStop = false;
  const char* reason = "IDLE";
};

class FanArbiter {
 public:
  FanArbiter(const FanDeviceConfig& deviceConfig, const FanAutomationConfig& automationConfig);
  void reset();
  FanArbiterResult evaluate(const FanArbiterInput& input);

 private:
  const FanDeviceConfig& deviceConfig_;
  const FanAutomationConfig& automationConfig_;
  unsigned long automaticRunSince_ = 0;
};

const char* fanArbiter_priorityName(FanDecisionPriority priority);
