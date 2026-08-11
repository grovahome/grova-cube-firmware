#include <Arduino.h>
#include "config.h"
#include "modules/ui.h"
#include "modules/stability.h"
#include "modules/climate.h"
#include "modules/rest_mode.h"
#include "modules/runtime_config.h"
#include "modules/sensors.h"
#include "modules/fan_control.h"

const int pwmChannel = 0;
const int pwmChannel2 = 1;
const int pwmFreq = 25000;
const int pwmResolution = 8;

struct FanChannel {
  int pwm = 0;
  int targetPercent = 0;
  int manualPercent = 50;
  int rpm = 0;
  int temperatureDemandPercent = 0;
  int humidityDemandPercent = 0;
  FanOperatingMode mode = FAN_MODE_AUTOMATIC;
  FanRuleState ruleState;
  bool tachoFault = false;
  unsigned long demandSince = 0;
  unsigned long autoRunSince = 0;
  unsigned long boostUntil = 0;
  const char* reason = "START";
};

static FanChannel fan1;
// Fan 2 is intentionally manual-only until per-channel automation rules exist.
// Starting at 0% keeps an unconnected or newly fitted fan in a safe state.
static FanChannel fan2;
static unsigned long lastTachoSample = 0;
static unsigned long lastFanLog = 0;
static unsigned long lastRampUpdate = 0;
static volatile unsigned long tachoPulseCount = 0;
static volatile unsigned long tacho2PulseCount = 0;
static const char* fanReasonName = "START";

static void IRAM_ATTR onFanTachoPulse() {
  tachoPulseCount++;
}

static void IRAM_ATTR onFan2TachoPulse() {
  tacho2PulseCount++;
}

static int clamp(int v, int minValue, int maxValue) {
  if (v < minValue) return minValue;
  if (v > maxValue) return maxValue;
  return v;
}

static int percentToPwm(int percent) {
  percent = clamp(percent, 0, 100);
  return map(percent, 0, 100, 0, 255);
}

static int pwmToPercent(int pwm) {
  return (clamp(pwm, 0, 255) * 100) / 255;
}

static bool isFanIndexEnabled(int fan) {
  if (fan == 1) return true;
  if (fan == 2) return FAN2_ENABLED;
  return false;
}

static FanChannel& channelForFan(int fan) {
  return fan == 2 ? fan2 : fan1;
}

static void writeFanPwm() {
  ledcWrite(pwmChannel, fan1.pwm);
#if FAN2_ENABLED
  ledcWrite(pwmChannel2, fan2.pwm);
#endif
}

void fan_preinit() {
  pinMode(FAN_PWM, OUTPUT);
  digitalWrite(FAN_PWM, LOW);
#if FAN2_ENABLED
  pinMode(FAN2_PWM, OUTPUT);
  digitalWrite(FAN2_PWM, LOW);
#endif
  fan1.pwm = 0;
  fan1.targetPercent = 0;
  fan1.manualPercent = fanControl_getConfig(1).manualPercent;
  fan1.mode = fanControl_getConfig(1).defaultMode;
  fan1.reason = fanControl_modeName(fan1.mode);
  fan2.pwm = 0;
  fan2.targetPercent = 0;
  fan2.manualPercent = fanControl_getConfig(2).manualPercent;
  fan2.mode = fanControl_getConfig(2).defaultMode;
  fan2.reason = fanControl_modeName(fan2.mode);
}

void fan_begin() {
  fan_preinit();
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(FAN_PWM, pwmChannel);
#if FAN2_ENABLED
  ledcSetup(pwmChannel2, pwmFreq, pwmResolution);
  ledcAttachPin(FAN2_PWM, pwmChannel2);
#endif
  writeFanPwm();

#if FAN_TACHO_ENABLED
  pinMode(FAN_TACHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(FAN_TACHO), onFanTachoPulse, FALLING);
#endif
#if FAN2_ENABLED && FAN2_TACHO_ENABLED
  pinMode(FAN2_TACHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(FAN2_TACHO), onFan2TachoPulse, FALLING);
#endif
#if FAN_TACHO_ENABLED || (FAN2_ENABLED && FAN2_TACHO_ENABLED)
  lastTachoSample = millis();
#endif
}

int getFanPercent() {
  return pwmToPercent(fan1.pwm);
}

int getFanTargetPercent() {
  return fan1.targetPercent;
}

int getFanRPM() {
  return fan1.rpm;
}

int getFan2RPM() {
  return fan2.rpm;
}

int getFan2Percent() {
  return pwmToPercent(fan2.pwm);
}

int getFan2TargetPercent() {
  return fan2.targetPercent;
}

int fan_getTemperatureDemandPercent(int fan) {
  if (!isFanIndexEnabled(fan)) return 0;
  return channelForFan(fan).temperatureDemandPercent;
}

int fan_getHumidityDemandPercent(int fan) {
  if (!isFanIndexEnabled(fan)) return 0;
  return channelForFan(fan).humidityDemandPercent;
}

bool fan_isEnabled(int fan) {
  return isFanIndexEnabled(fan);
}

bool fan_isManual(int fan) {
  if (!isFanIndexEnabled(fan)) return false;
  return channelForFan(fan).mode == FAN_MODE_MANUAL;
}

bool fan_setAuto(int fan) {
  if (fan == 0) {
    fan1.tachoFault = false;
    fan1.demandSince = 0;
    fan1.autoRunSince = 0;
    fan1.boostUntil = 0;
    fan1.mode = FAN_MODE_AUTOMATIC;
    return true;
  }

  if (!isFanIndexEnabled(fan)) return false;
  FanChannel& channel = channelForFan(fan);
  channel.tachoFault = false;
  channel.demandSince = 0;
  channel.autoRunSince = 0;
  channel.boostUntil = 0;
  channel.mode = FAN_MODE_AUTOMATIC;
  return true;
}

bool fan_setManual(int fan, int percent) {
  percent = clamp(percent, 0, 100);

  if (fan == 0) {
    fan1.tachoFault = false;
    fan1.demandSince = 0;
    fan1.autoRunSince = 0;
    fan1.boostUntil = 0;
    fan1.mode = FAN_MODE_MANUAL;
    fan1.manualPercent = percent;
    return true;
  }

  if (!isFanIndexEnabled(fan)) return false;
  FanChannel& channel = channelForFan(fan);
  channel.tachoFault = false;
  channel.demandSince = 0;
  channel.autoRunSince = 0;
  channel.boostUntil = 0;
  channel.mode = FAN_MODE_MANUAL;
  channel.manualPercent = percent;
  return true;
}

bool fan_getTachoFault(int fan) {
  if (!isFanIndexEnabled(fan)) return false;
  return channelForFan(fan).tachoFault;
}

bool fan_hasTachoFault() {
  return fan1.tachoFault || fan2.tachoFault;
}

const char* fan_getTachoStatusName() {
  if (fan1.tachoFault) return "F1 FAULT";
  if (fan2.tachoFault) return "F2 FAULT";
#if !FAN_TACHO_ENABLED && !(FAN2_ENABLED && FAN2_TACHO_ENABLED)
  return "OFF";
#else
  return "OK";
#endif
}

const char* fan_getModeName(int fan) {
  if (!isFanIndexEnabled(fan)) return "OFF";
  if (restMode_isEnabled()) return "REST";
  if (channelForFan(fan).tachoFault) return "FAULT";
  if (channelForFan(fan).mode == FAN_MODE_OFF) return "OFF";
  if (channelForFan(fan).mode == FAN_MODE_MANUAL) return "MANUAL";
  if (fan == 1 && ui_isFanManual()) return "MANUAL";
  return "AUTO";
}

const char* fan_getReasonName() {
  return fanReasonName;
}

const char* fan_getReasonName(int fan) {
  if (!isFanIndexEnabled(fan)) return "OFF";
  if (restMode_isEnabled()) return "REST OFF";
  return channelForFan(fan).reason;
}

void fan_forceOff() {
  fanReasonName = "REST OFF";
  fan1.targetPercent = 0;
  fan1.pwm = 0;
  fan1.reason = "REST OFF";
  fan1.demandSince = 0;
#if FAN2_ENABLED
  fan2.targetPercent = 0;
  fan2.pwm = 0;
  fan2.reason = "REST OFF";
  fan2.demandSince = 0;
#else
  fan2.targetPercent = 0;
  fan2.pwm = 0;
  fan2.reason = "OFF";
  fan2.demandSince = 0;
  fan2.tachoFault = false;
#endif
  writeFanPwm();
}

static void rampFanToTarget(FanChannel& channel, int targetPWM, const FanControlConfig& config) {
  if (channel.pwm < targetPWM) channel.pwm += config.rampUpPwmStep;
  if (channel.pwm > targetPWM) channel.pwm -= config.rampDownPwmStep;
  channel.pwm = clamp(channel.pwm, 0, 255);
}

static int applyMinimumIfRunning(int requested, const FanControlConfig& config) {
  if (requested <= 0) return 0;
  return clamp(requested, config.minimumPercent, config.maximumPercent);
}

static void applyFanChannel(FanChannel& channel, int fan, const FanDemand& demand, bool sensorFault) {
  const FanControlConfig& config = fanControl_getConfig(fan);
  const unsigned long now = millis();
  if (channel.mode != FAN_MODE_AUTOMATIC || sensorFault || channel.tachoFault) {
    channel.temperatureDemandPercent = 0;
    channel.humidityDemandPercent = 0;
  }
  if (channel.tachoFault) {
    channel.reason = "STALL OFF";
    channel.targetPercent = 0;
  } else if (sensorFault && channel.mode == FAN_MODE_AUTOMATIC) {
    channel.reason = "SENSOR SAFE";
    channel.targetPercent = clamp(FAN_SENSOR_FAIL_PERCENT, config.minimumPercent, config.maximumPercent);
  } else if (channel.mode == FAN_MODE_OFF) {
    channel.reason = "OFF";
    channel.targetPercent = 0;
  } else if (channel.mode == FAN_MODE_MANUAL || (fan == 1 && ui_isFanManual())) {
    channel.reason = "MANUAL";
    const int requested = channel.mode == FAN_MODE_MANUAL ? channel.manualPercent : ui_getFanManualValue();
    channel.targetPercent = applyMinimumIfRunning(requested, config);
  } else {
    channel.temperatureDemandPercent = demand.temperaturePercent;
    channel.humidityDemandPercent = demand.humidityPercent;
    int requested = demand.percent;

    bool minimumRunHold = false;
    if (requested > 0) {
      if (channel.autoRunSince == 0) {
        channel.autoRunSince = now;
        channel.boostUntil = now + config.startupBoostMs;
        channel.pwm = percentToPwm(config.startupBoostPercent);
      }
    } else if (channel.autoRunSince != 0 && now - channel.autoRunSince < config.minimumRunMs) {
      requested = config.minimumPercent;
      minimumRunHold = true;
    } else {
      channel.autoRunSince = 0;
      channel.boostUntil = 0;
    }

    channel.reason = minimumRunHold ? "MIN RUN" : demand.reason;
    channel.targetPercent = applyMinimumIfRunning(requested, config);
    if (channel.boostUntil != 0 && static_cast<long>(channel.boostUntil - now) > 0) {
      channel.targetPercent = max(channel.targetPercent, config.startupBoostPercent);
      channel.reason = "START BOOST";
    } else if (channel.boostUntil != 0) {
      channel.boostUntil = 0;
    }
  }

  channel.targetPercent = clamp(channel.targetPercent, 0, config.maximumPercent);
}

static void updateFanTacho() {
  unsigned long now = millis();

#if !FAN_TACHO_ENABLED && !(FAN2_ENABLED && FAN2_TACHO_ENABLED)
  fan1.rpm = 0;
  fan2.rpm = 0;
  fan1.tachoFault = false;
  fan2.tachoFault = false;
  return;
#else
  unsigned long elapsed = now - lastTachoSample;

  if (elapsed >= FAN_TACHO_SAMPLE_MS) {
    noInterrupts();
    unsigned long pulses = tachoPulseCount;
    unsigned long pulses2 = tacho2PulseCount;
    tachoPulseCount = 0;
    tacho2PulseCount = 0;
    interrupts();

    if (FAN_TACHO_PULSES_PER_REV > 0 && elapsed > 0) {
#if FAN_TACHO_ENABLED
      fan1.rpm = (pulses * 60000UL) / elapsed / FAN_TACHO_PULSES_PER_REV;
#else
      fan1.rpm = 0;
#endif
#if FAN2_ENABLED && FAN2_TACHO_ENABLED
      fan2.rpm = (pulses2 * 60000UL) / elapsed / FAN_TACHO_PULSES_PER_REV;
#else
      fan2.rpm = 0;
#endif
    } else {
      fan1.rpm = 0;
      fan2.rpm = 0;
    }

    lastTachoSample = now;
  }

  FanChannel* channels[] = {&fan1, &fan2};
  for (int i = 0; i < 2; i++) {
    bool tachoEnabled = (i == 0 && FAN_TACHO_ENABLED) || (i == 1 && FAN2_ENABLED && FAN2_TACHO_ENABLED);
    if (channels[i]->tachoFault) {
      channels[i]->targetPercent = 0;
      channels[i]->reason = "STALL OFF";
      channels[i]->demandSince = 0;
      continue;
    }
    const FanControlConfig& config = fanControl_getConfig(i + 1);
    bool shouldSpin = tachoEnabled && channels[i]->targetPercent >= config.stall.minimumCheckPercent;

    if (!shouldSpin) {
      channels[i]->demandSince = 0;
      continue;
    }

    if (channels[i]->demandSince == 0) {
      channels[i]->demandSince = now;
      channels[i]->tachoFault = false;
      continue;
    }

    if (now - channels[i]->demandSince >= config.stall.faultDelayMs) {
      if (channels[i]->rpm < config.stall.minimumRpm) {
        channels[i]->tachoFault = true;
        channels[i]->targetPercent = 0;
        channels[i]->pwm = 0;
        channels[i]->reason = "STALL OFF";
      }
    }
  }
  // A detected stall is a safety shutdown and must not wait for the normal
  // downward ramp, which is intentionally slow during regular operation.
  writeFanPwm();
#endif
}

void fan_loop(float t, float h) {
  if (restMode_isEnabled()) {
    fan_forceOff();
    updateFanTacho();
    return;
  }

  t = safeTemp(t);
  h = safeHum(h);

  bool sensorFault = sensors_hasFault();
  FanDemand fan1Demand = fanControl_evaluate(1, t, h, getTargetTemp(), getTargetHum(), fan1.ruleState);
  FanDemand fan2Demand = fanControl_evaluate(2, t, h, getTargetTemp(), getTargetHum(), fan2.ruleState);
  fanReasonName = sensorFault ? "SENSOR SAFE" : fan1Demand.reason;

  applyFanChannel(fan1, 1, fan1Demand, sensorFault);
#if FAN2_ENABLED
  applyFanChannel(fan2, 2, fan2Demand, sensorFault);
#else
  fan2.targetPercent = 0;
  fan2.pwm = 0;
  fan2.reason = "OFF";
#endif
  fanReasonName = fan1.reason;

  unsigned long now = millis();
  const unsigned long rampInterval = min(
    fanControl_getConfig(1).rampIntervalMs,
    fanControl_getConfig(2).rampIntervalMs
  );
  if (now - lastRampUpdate >= rampInterval) {
    lastRampUpdate = now;
    rampFanToTarget(fan1, percentToPwm(fan1.targetPercent), fanControl_getConfig(1));
#if FAN2_ENABLED
    rampFanToTarget(fan2, percentToPwm(fan2.targetPercent), fanControl_getConfig(2));
#endif
  }

  writeFanPwm();
  updateFanTacho();
  if (fan1.tachoFault) fanReasonName = "STALL OFF";

  if (millis() - lastFanLog >= FAN_LOG_INTERVAL_MS) {
    lastFanLog = millis();
    Serial.print("Fan ");
    Serial.print(sensorFault ? "SAFE" : "AUTO");
    Serial.print(" | F1 ");
    Serial.print(getFanPercent());
    Serial.print("/");
    Serial.print(getFanTargetPercent());
    Serial.print("%");
#if FAN2_ENABLED
    Serial.print(" | F2 ");
    Serial.print(getFan2Percent());
    Serial.print("/");
    Serial.print(getFan2TargetPercent());
    Serial.print("%");
#endif
    Serial.print(" | rpm ");
    Serial.print(getFanRPM());
#if FAN2_ENABLED
    Serial.print("/");
    Serial.print(getFan2RPM());
#endif
    Serial.print(" | tach ");
    Serial.println(fan_getTachoStatusName());
  }
}
