#include <Arduino.h>
#include "modules/fan_device.h"
#include "modules/fan_hw_driver.h"

static int percentToPwm(int percent) {
  return map(constrain(percent, 0, 100), 0, 100, 0, 255);
}

static int pwmToPercent(int pwm) {
  return (constrain(pwm, 0, 255) * 100) / 255;
}

FanDevice::FanDevice(int fan, const FanDeviceConfig& config)
  : fan_(fan), config_(config) {}

void FanDevice::begin() {
  requestedPercent_ = 0;
  appliedPwm_ = 0;
  faultLatched_ = false;
  spinDemandSince_ = 0;
  boostUntil_ = 0;
  lastRampUpdate_ = millis();
  fanHw_emergencyStop(fan_);
}

void FanDevice::requestPercent(int percent) {
  if (faultLatched_) {
    requestedPercent_ = 0;
    return;
  }

  percent = constrain(percent, 0, config_.maximumPercent);
  if (percent > 0 && requestedPercent_ == 0) {
    boostUntil_ = millis() + config_.startupBoostMs;
  }
  if (percent == 0) boostUntil_ = 0;
  requestedPercent_ = percent;
}

void FanDevice::forceOff() {
  requestedPercent_ = 0;
  appliedPwm_ = 0;
  spinDemandSince_ = 0;
  boostUntil_ = 0;
  fanHw_emergencyStop(fan_);
}

void FanDevice::acknowledgeFault() {
  faultLatched_ = false;
  spinDemandSince_ = 0;
  boostUntil_ = 0;
}

void FanDevice::update() {
  fanHw_update();
  const unsigned long now = millis();

  if (faultLatched_) {
    forceOff();
    return;
  }

  int effectivePercent = requestedPercent_;
  if (boostUntil_ != 0 && static_cast<long>(boostUntil_ - now) > 0) {
    effectivePercent = max(effectivePercent, config_.startupBoostPercent);
  } else if (boostUntil_ != 0) {
    boostUntil_ = 0;
  }

  const int targetPwm = percentToPwm(effectivePercent);
  if (now - lastRampUpdate_ >= config_.rampIntervalMs) {
    lastRampUpdate_ = now;
    if (appliedPwm_ < targetPwm) appliedPwm_ += config_.rampUpPwmStep;
    if (appliedPwm_ > targetPwm) appliedPwm_ -= config_.rampDownPwmStep;
    appliedPwm_ = constrain(appliedPwm_, 0, 255);
    fanHw_writePwm(fan_, appliedPwm_);
  }

  const bool shouldSpin = fanHw_isTachoEnabled(fan_) &&
    requestedPercent_ >= config_.stall.minimumCheckPercent;
  if (!shouldSpin) {
    spinDemandSince_ = 0;
    return;
  }

  if (spinDemandSince_ == 0) {
    spinDemandSince_ = now;
    return;
  }

  if (now - spinDemandSince_ >= config_.stall.faultDelayMs &&
      fanHw_getRpm(fan_) < config_.stall.minimumRpm) {
    faultLatched_ = true;
    requestedPercent_ = 0;
    appliedPwm_ = 0;
    fanHw_emergencyStop(fan_);
  }
}

FanDeviceStatus FanDevice::getStatus() const {
  FanDeviceStatus status;
  status.requestedPercent = requestedPercent_;
  status.appliedPercent = pwmToPercent(fanHw_getPwm(fan_));
  status.rpm = fanHw_getRpm(fan_);
  status.starting = boostUntil_ != 0 && static_cast<long>(boostUntil_ - millis()) > 0;
  status.stalled = faultLatched_;
  status.faultLatched = faultLatched_;
  return status;
}
