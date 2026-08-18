#include <Arduino.h>
#include "config.h"
#include "modules/climate.h"
#include "modules/fan.h"
#include "modules/fan_arbiter.h"
#include "modules/fan_control.h"
#include "modules/fan_device.h"
#include "modules/fan_hw_driver.h"
#include "modules/rest_mode.h"
#include "modules/sensors.h"
#include "modules/ui.h"

struct FanChannelControl {
  int manualPercent = 0;
  FanDemand automaticDemand;
  FanOperatingMode mode = FAN_MODE_AUTOMATIC;
  FanRuleState ruleState;
  FanDecisionPriority decisionPriority = FAN_PRIORITY_IDLE;
  const char* reason = "START";
};

static FanChannelControl fanControls[FAN_CHANNEL_COUNT];
static FanDevice fanDevices[FAN_CHANNEL_COUNT] = {
  FanDevice(1, fanControl_getDeviceConfig(1)),
  FanDevice(2, fanControl_getDeviceConfig(2))
};
static FanArbiter fanArbiters[FAN_CHANNEL_COUNT] = {
  FanArbiter(fanControl_getDeviceConfig(1), fanControl_getAutomationConfig(1)),
  FanArbiter(fanControl_getDeviceConfig(2), fanControl_getAutomationConfig(2))
};
static unsigned long lastFanLog = 0;
static const char* fanReasonName = "START";

static int indexFor(int fan) {
  return fan >= 1 && fan <= FAN_CHANNEL_COUNT ? fan - 1 : 0;
}

static FanChannelControl& controlFor(int fan) {
  return fanControls[indexFor(fan)];
}
static FanDevice& deviceFor(int fan) {
  return fanDevices[indexFor(fan)];
}
static FanArbiter& arbiterFor(int fan) {
  return fanArbiters[indexFor(fan)];
}

static void resetControl(FanChannelControl& control, int fan) {
  const FanAutomationConfig& config = fanControl_getAutomationConfig(fan);
  control.manualPercent = config.manualPercent;
  control.automaticDemand = FanDemand{};
  control.mode = config.defaultMode;
  control.ruleState = FanRuleState{};
  control.decisionPriority = FAN_PRIORITY_IDLE;
  control.reason = fanControl_modeName(control.mode);
  arbiterFor(fan).reset();
}

void fan_preinit() {
  fanHw_preinit();
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    resetControl(controlFor(fan), fan);
  }
}

void fan_begin() {
  fanHw_begin();
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    resetControl(controlFor(fan), fan);
    deviceFor(fan).begin();
  }
}

int getFanPercent() { return deviceFor(1).getStatus().appliedPercent; }
int getFanTargetPercent() { return deviceFor(1).getStatus().requestedPercent; }
int getFanRPM() { return deviceFor(1).getStatus().rpm; }
int getFan2RPM() { return deviceFor(2).getStatus().rpm; }
int getFan2Percent() { return deviceFor(2).getStatus().appliedPercent; }
int getFan2TargetPercent() { return deviceFor(2).getStatus().requestedPercent; }

int fan_getTemperatureDemandPercent(int fan) {
  return fanHw_isEnabled(fan)
    ? controlFor(fan).automaticDemand.temperaturePercent
    : 0;
}

int fan_getHumidityDemandPercent(int fan) {
  return fanHw_isEnabled(fan)
    ? controlFor(fan).automaticDemand.humidityPercent
    : 0;
}

int fan_getRuleDemandCount(int fan) {
  return fanHw_isEnabled(fan) ? controlFor(fan).automaticDemand.ruleCount : 0;
}

int fan_getRuleDemandPercent(int fan, int ruleIndex) {
  if (!fanHw_isEnabled(fan)) return 0;
  const FanDemand& demand = controlFor(fan).automaticDemand;
  if (ruleIndex < 0 || ruleIndex >= demand.ruleCount) return 0;
  return demand.rules[ruleIndex].percent;
}

const char* fan_getRuleDemandId(int fan, int ruleIndex) {
  if (!fanHw_isEnabled(fan)) return "NONE";
  const FanDemand& demand = controlFor(fan).automaticDemand;
  if (ruleIndex < 0 || ruleIndex >= demand.ruleCount) return "NONE";
  return demand.rules[ruleIndex].ruleId;
}

const char* fan_getWinningRuleId(int fan) {
  if (!fanHw_isEnabled(fan)) return "NONE";
  const FanDemand& demand = controlFor(fan).automaticDemand;
  if (demand.winningRuleIndex < 0 || demand.winningRuleIndex >= demand.ruleCount) {
    return demand.percent > 0 ? "BASE" : "NONE";
  }
  return demand.rules[demand.winningRuleIndex].ruleId;
}

const char* fan_getDecisionPriorityName(int fan) {
  return fanHw_isEnabled(fan)
    ? fanArbiter_priorityName(controlFor(fan).decisionPriority)
    : "DISABLED";
}

bool fan_isEnabled(int fan) { return fanHw_isEnabled(fan); }

bool fan_isManual(int fan) {
  return fanHw_isEnabled(fan) && controlFor(fan).mode == FAN_MODE_MANUAL;
}

bool fan_setAuto(int fan) {
  if (fan == 0) fan = 1;
  if (!fanHw_isEnabled(fan)) return false;
  FanChannelControl& control = controlFor(fan);
  FanDevice& device = deviceFor(fan);
  device.acknowledgeFault();
  device.forceOff();
  arbiterFor(fan).reset();
  control.mode = FAN_MODE_AUTOMATIC;
  control.decisionPriority = FAN_PRIORITY_IDLE;
  control.reason = "IDLE";
  return true;
}

bool fan_setManual(int fan, int percent) {
  if (fan == 0) fan = 1;
  if (!fanHw_isEnabled(fan) || percent < 0 || percent > 100) return false;
  FanChannelControl& control = controlFor(fan);
  FanDevice& device = deviceFor(fan);
  device.acknowledgeFault();
  device.forceOff();
  arbiterFor(fan).reset();
  control.mode = FAN_MODE_MANUAL;
  control.manualPercent = percent;
  control.decisionPriority = FAN_PRIORITY_MANUAL;
  control.reason = "MANUAL";
  return true;
}

bool fan_getTachoFault(int fan) {
  return fanHw_isEnabled(fan) && deviceFor(fan).getStatus().faultLatched;
}

bool fan_hasTachoFault() {
  return fan_getTachoFault(1) || fan_getTachoFault(2);
}

const char* fan_getTachoStatusName() {
  if (fan_getTachoFault(1)) return "F1 FAULT";
  if (fan_getTachoFault(2)) return "F2 FAULT";
  if (!fanHw_isTachoEnabled(1) && !fanHw_isTachoEnabled(2)) return "OFF";
  return "OK";
}

const char* fan_getModeName(int fan) {
  if (!fanHw_isEnabled(fan)) return "OFF";
  if (restMode_isEnabled()) return "REST";
  if (fan_getTachoFault(fan)) return "FAULT";
  const FanChannelControl& control = controlFor(fan);
  if (control.mode == FAN_MODE_OFF) return "OFF";
  if (control.mode == FAN_MODE_MANUAL) return "MANUAL";
  if (fan == 1 && ui_isFanManual()) return "MANUAL";
  return "AUTO";
}

const char* fan_getReasonName() { return fanReasonName; }

const char* fan_getReasonName(int fan) {
  if (!fanHw_isEnabled(fan)) return "OFF";
  if (restMode_isEnabled()) return "REST OFF";
  if (fan_getTachoFault(fan)) return "STALL OFF";
  return controlFor(fan).reason;
}

void fan_forceOff() {
  fanReasonName = "REST OFF";
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    FanChannelControl& control = controlFor(fan);
    const bool enabled = fanHw_isEnabled(fan);
    control.reason = enabled ? "REST OFF" : "OFF";
    control.decisionPriority = enabled ? FAN_PRIORITY_REST : FAN_PRIORITY_IDLE;
    arbiterFor(fan).reset();
    deviceFor(fan).forceOff();
  }
}

static FanArbiterResult evaluateFanDecision(
  int fan,
  FanChannelControl& control,
  const FanDemand& demand,
  bool sensorFault
) {
  FanArbiterInput input;
  input.mode = control.mode;
  input.manualPercent = control.manualPercent;
  input.legacyManualOverride = fan == 1 && ui_isFanManual();
  input.legacyManualPercent = fan == 1 ? ui_getFanManualValue() : 0;
  input.restMode = restMode_isEnabled();
  input.sensorFault = sensorFault;
  input.deviceFault = fan_getTachoFault(fan);
  input.automaticDemand = demand;
  return arbiterFor(fan).evaluate(input);
}

void fan_loop() {
  const bool sensorFault = sensors_hasFault();
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    FanChannelControl& control = controlFor(fan);
    FanDevice& device = deviceFor(fan);
    const FanDemand demand = fanControl_evaluate(
      fan, getTargetTemp(), getTargetHum(), control.ruleState);
    const FanArbiterResult decision = evaluateFanDecision(
      fan, control, demand, sensorFault);

    control.automaticDemand = decision.automaticDemand;
    control.reason = decision.reason;
    control.decisionPriority = decision.priority;

    if (!fanHw_isEnabled(fan) || decision.immediateStop) {
      device.forceOff();
    } else {
      device.requestPercent(decision.requestedPercent);
    }
    device.update();
    if (fan_getTachoFault(fan)) control.reason = "STALL OFF";
  }
  fanReasonName = controlFor(1).reason;

  if (millis() - lastFanLog >= FAN_LOG_INTERVAL_MS) {
    lastFanLog = millis();
    Serial.print("Fan | F1 ");
    Serial.print(getFanPercent());
    Serial.print("/");
    Serial.print(getFanTargetPercent());
    Serial.print("% | F2 ");
    Serial.print(getFan2Percent());
    Serial.print("/");
    Serial.print(getFan2TargetPercent());
    Serial.print("% | rpm ");
    Serial.print(getFanRPM());
    Serial.print("/");
    Serial.print(getFan2RPM());
    Serial.print(" | tach ");
    Serial.println(fan_getTachoStatusName());
  }
}
