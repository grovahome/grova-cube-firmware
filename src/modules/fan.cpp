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
#include "modules/stability.h"
#include "modules/ui.h"

struct FanChannelControl {
  int manualPercent = 0;
  int temperatureDemandPercent = 0;
  int humidityDemandPercent = 0;
  FanOperatingMode mode = FAN_MODE_AUTOMATIC;
  FanRuleState ruleState;
  FanDecisionPriority decisionPriority = FAN_PRIORITY_IDLE;
  const char* reason = "START";
};

static FanChannelControl fan1Control;
static FanChannelControl fan2Control;
static FanDevice fan1Device(1, fanControl_getDeviceConfig(1));
static FanDevice fan2Device(2, fanControl_getDeviceConfig(2));
static FanArbiter fan1Arbiter(
  fanControl_getDeviceConfig(1), fanControl_getAutomationConfig(1));
static FanArbiter fan2Arbiter(
  fanControl_getDeviceConfig(2), fanControl_getAutomationConfig(2));
static unsigned long lastFanLog = 0;
static const char* fanReasonName = "START";

static FanChannelControl& controlFor(int fan) {
  return fan == 2 ? fan2Control : fan1Control;
}
static FanDevice& deviceFor(int fan) {
  return fan == 2 ? fan2Device : fan1Device;
}
static FanArbiter& arbiterFor(int fan) {
  return fan == 2 ? fan2Arbiter : fan1Arbiter;
}

static void resetControl(FanChannelControl& control, int fan) {
  const FanAutomationConfig& config = fanControl_getAutomationConfig(fan);
  control.manualPercent = config.manualPercent;
  control.temperatureDemandPercent = 0;
  control.humidityDemandPercent = 0;
  control.mode = config.defaultMode;
  control.ruleState = FanRuleState{};
  control.decisionPriority = FAN_PRIORITY_IDLE;
  control.reason = fanControl_modeName(control.mode);
  arbiterFor(fan).reset();
}

void fan_preinit() {
  fanHw_preinit();
  resetControl(fan1Control, 1);
  resetControl(fan2Control, 2);
}

void fan_begin() {
  fanHw_begin();
  resetControl(fan1Control, 1);
  resetControl(fan2Control, 2);
  fan1Device.begin();
  fan2Device.begin();
}

int getFanPercent() { return fan1Device.getStatus().appliedPercent; }
int getFanTargetPercent() { return fan1Device.getStatus().requestedPercent; }
int getFanRPM() { return fan1Device.getStatus().rpm; }
int getFan2RPM() { return fan2Device.getStatus().rpm; }
int getFan2Percent() { return fan2Device.getStatus().appliedPercent; }
int getFan2TargetPercent() { return fan2Device.getStatus().requestedPercent; }

int fan_getTemperatureDemandPercent(int fan) {
  return fanHw_isEnabled(fan) ? controlFor(fan).temperatureDemandPercent : 0;
}

int fan_getHumidityDemandPercent(int fan) {
  return fanHw_isEnabled(fan) ? controlFor(fan).humidityDemandPercent : 0;
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
  fan1Control.reason = "REST OFF";
  fan2Control.reason = FAN2_ENABLED ? "REST OFF" : "OFF";
  fan1Control.decisionPriority = FAN_PRIORITY_REST;
  fan2Control.decisionPriority = FAN2_ENABLED ? FAN_PRIORITY_REST : FAN_PRIORITY_IDLE;
  fan1Arbiter.reset();
  fan2Arbiter.reset();
  fan1Device.forceOff();
  fan2Device.forceOff();
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

void fan_loop(float temperature, float humidity) {
  temperature = safeTemp(temperature);
  humidity = safeHum(humidity);
  const bool sensorFault = sensors_hasFault();
  const FanDemand demand1 = fanControl_evaluate(
    1, temperature, humidity, getTargetTemp(), getTargetHum(), fan1Control.ruleState);
  const FanDemand demand2 = fanControl_evaluate(
    2, temperature, humidity, getTargetTemp(), getTargetHum(), fan2Control.ruleState);

  const FanArbiterResult decision1 = evaluateFanDecision(1, fan1Control, demand1, sensorFault);
  const FanArbiterResult decision2 = evaluateFanDecision(2, fan2Control, demand2, sensorFault);
  fan1Control.temperatureDemandPercent = decision1.temperatureDemandPercent;
  fan1Control.humidityDemandPercent = decision1.humidityDemandPercent;
  fan1Control.reason = decision1.reason;
  fan1Control.decisionPriority = decision1.priority;
  fan2Control.temperatureDemandPercent = decision2.temperatureDemandPercent;
  fan2Control.humidityDemandPercent = decision2.humidityDemandPercent;
  fan2Control.reason = decision2.reason;
  fan2Control.decisionPriority = decision2.priority;

  if (decision1.immediateStop) fan1Device.forceOff();
  else fan1Device.requestPercent(decision1.requestedPercent);
#if FAN2_ENABLED
  if (decision2.immediateStop) fan2Device.forceOff();
  else fan2Device.requestPercent(decision2.requestedPercent);
#else
  fan2Device.forceOff();
#endif

  fan1Device.update();
  fan2Device.update();
  if (fan_getTachoFault(1)) fan1Control.reason = "STALL OFF";
  if (fan_getTachoFault(2)) fan2Control.reason = "STALL OFF";
  fanReasonName = fan1Control.reason;

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
