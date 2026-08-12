#include <Arduino.h>
#include "config.h"
#include "modules/climate.h"
#include "modules/fan.h"
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
  unsigned long autoRunSince = 0;
  const char* reason = "START";
};

static FanChannelControl fan1Control;
static FanChannelControl fan2Control;
static FanDevice fan1Device(1, fanControl_getConfig(1));
static FanDevice fan2Device(2, fanControl_getConfig(2));
static unsigned long lastFanLog = 0;
static const char* fanReasonName = "START";

static FanChannelControl& controlFor(int fan) {
  return fan == 2 ? fan2Control : fan1Control;
}
static FanDevice& deviceFor(int fan) {
  return fan == 2 ? fan2Device : fan1Device;
}

static int applyMinimumIfRunning(int requested, const FanControlConfig& config) {
  if (requested <= 0) return 0;
  return constrain(requested, config.minimumPercent, config.maximumPercent);
}

static void resetControl(FanChannelControl& control, int fan) {
  const FanControlConfig& config = fanControl_getConfig(fan);
  control.manualPercent = config.manualPercent;
  control.temperatureDemandPercent = 0;
  control.humidityDemandPercent = 0;
  control.mode = config.defaultMode;
  control.ruleState = FanRuleState{};
  control.autoRunSince = 0;
  control.reason = fanControl_modeName(control.mode);
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
  control.autoRunSince = 0;
  control.mode = FAN_MODE_AUTOMATIC;
  return true;
}

bool fan_setManual(int fan, int percent) {
  if (fan == 0) fan = 1;
  if (!fanHw_isEnabled(fan) || percent < 0 || percent > 100) return false;
  FanChannelControl& control = controlFor(fan);
  FanDevice& device = deviceFor(fan);
  device.acknowledgeFault();
  device.forceOff();
  control.autoRunSince = 0;
  control.mode = FAN_MODE_MANUAL;
  control.manualPercent = percent;
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
  fan1Control.autoRunSince = 0;
  fan2Control.autoRunSince = 0;
  fan1Device.forceOff();
  fan2Device.forceOff();
}

static int evaluateRequestedPercent(
  FanChannelControl& control,
  int fan,
  const FanDemand& demand,
  bool sensorFault
) {
  const FanControlConfig& config = fanControl_getConfig(fan);
  const unsigned long now = millis();
  control.temperatureDemandPercent = 0;
  control.humidityDemandPercent = 0;

  if (fan_getTachoFault(fan)) {
    control.reason = "STALL OFF";
    return 0;
  }
  if (sensorFault && control.mode == FAN_MODE_AUTOMATIC) {
    control.reason = "SENSOR SAFE";
    return constrain(FAN_SENSOR_FAIL_PERCENT, config.minimumPercent, config.maximumPercent);
  }
  if (control.mode == FAN_MODE_OFF) {
    control.reason = "OFF";
    return 0;
  }
  if (control.mode == FAN_MODE_MANUAL || (fan == 1 && ui_isFanManual())) {
    control.reason = "MANUAL";
    const int requested = control.mode == FAN_MODE_MANUAL
      ? control.manualPercent
      : ui_getFanManualValue();
    return applyMinimumIfRunning(requested, config);
  }

  control.temperatureDemandPercent = demand.temperaturePercent;
  control.humidityDemandPercent = demand.humidityPercent;
  int requested = demand.percent;
  if (requested > 0) {
    if (control.autoRunSince == 0) control.autoRunSince = now;
    control.reason = demand.reason;
  } else if (control.autoRunSince != 0 && now - control.autoRunSince < config.minimumRunMs) {
    requested = config.minimumPercent;
    control.reason = "MIN RUN";
  } else {
    control.autoRunSince = 0;
    control.reason = demand.reason;
  }
  return applyMinimumIfRunning(requested, config);
}

void fan_loop(float temperature, float humidity) {
  if (restMode_isEnabled()) {
    fan_forceOff();
    fanHw_update();
    return;
  }

  temperature = safeTemp(temperature);
  humidity = safeHum(humidity);
  const bool sensorFault = sensors_hasFault();
  const FanDemand demand1 = fanControl_evaluate(
    1, temperature, humidity, getTargetTemp(), getTargetHum(), fan1Control.ruleState);
  const FanDemand demand2 = fanControl_evaluate(
    2, temperature, humidity, getTargetTemp(), getTargetHum(), fan2Control.ruleState);

  fan1Device.requestPercent(evaluateRequestedPercent(fan1Control, 1, demand1, sensorFault));
#if FAN2_ENABLED
  fan2Device.requestPercent(evaluateRequestedPercent(fan2Control, 2, demand2, sensorFault));
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
