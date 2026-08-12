#pragma once

#include "modules/fan_control.h"

struct FanDeviceStatus {
  int requestedPercent = 0;
  int appliedPercent = 0;
  int rpm = 0;
  bool starting = false;
  bool stalled = false;
  bool faultLatched = false;
};

class FanDevice {
 public:
  FanDevice(int fan, const FanControlConfig& config);
  void begin();
  void requestPercent(int percent);
  void forceOff();
  void acknowledgeFault();
  void update();
  FanDeviceStatus getStatus() const;

 private:
  int fan_;
  const FanControlConfig& config_;
  int requestedPercent_ = 0;
  int appliedPwm_ = 0;
  bool faultLatched_ = false;
  unsigned long spinDemandSince_ = 0;
  unsigned long boostUntil_ = 0;
  unsigned long lastRampUpdate_ = 0;
};

