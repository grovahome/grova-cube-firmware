#include <Arduino.h>
#include "config.h"
#include "modules/ui.h"
#include "modules/stability.h"
#include "modules/climate.h"
#include "modules/runtime_config.h"
#include "modules/sensors.h"

const int pwmChannel = 0;
const int pwmChannel2 = 1;
const int pwmFreq = 25000;
const int pwmResolution = 8;

static int currentPWM = 0;
static int targetFanPercent = 0;
static int fanRPM = 0;
static int fan2RPM = 0;
static bool tachoFault = false;
static bool fan2TachoFault = false;
static unsigned long lastTachoSample = 0;
static unsigned long fanDemandSince = 0;
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

static void writeFanPwm() {
  ledcWrite(pwmChannel, currentPWM);
#if FAN2_ENABLED
  ledcWrite(pwmChannel2, currentPWM);
#endif
}

void fan_preinit() {
  pinMode(FAN_PWM, OUTPUT);
  digitalWrite(FAN_PWM, LOW);
#if FAN2_ENABLED
  pinMode(FAN2_PWM, OUTPUT);
  digitalWrite(FAN2_PWM, LOW);
#endif
  currentPWM = 0;
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
#if FAN2_ENABLED
  pinMode(FAN2_TACHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(FAN2_TACHO), onFan2TachoPulse, FALLING);
#endif
  lastTachoSample = millis();
#endif
}

int getFanPercent() {
  return (currentPWM * 100) / 255;
}

int getFanTargetPercent() {
  return targetFanPercent;
}

int getFanRPM() {
  return fanRPM;
}

int getFan2RPM() {
  return fan2RPM;
}

bool fan_hasTachoFault() {
  return tachoFault || fan2TachoFault;
}

const char* fan_getTachoStatusName() {
#if !FAN_TACHO_ENABLED
  return "OFF";
#else
  if (tachoFault) return "F1 FAULT";
  if (fan2TachoFault) return "F2 FAULT";
  return "OK";
#endif
}

const char* fan_getReasonName() {
  return fanReasonName;
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

static void rampFanToTarget(int targetPWM) {
  unsigned long now = millis();

  if (now - lastRampUpdate < FAN_RAMP_INTERVAL_MS) return;
  lastRampUpdate = now;

  if (currentPWM < targetPWM) currentPWM += FAN_RAMP_UP_PWM_STEP;
  if (currentPWM > targetPWM) currentPWM -= FAN_RAMP_DOWN_PWM_STEP;

  currentPWM = clamp(currentPWM, 0, 255);
}

static void updateFanTacho() {
#if !FAN_TACHO_ENABLED
  fanRPM = 0;
  fan2RPM = 0;
  tachoFault = false;
  fan2TachoFault = false;
  return;
#else
  unsigned long now = millis();
  unsigned long elapsed = now - lastTachoSample;

  if (elapsed >= FAN_TACHO_SAMPLE_MS) {
    noInterrupts();
    unsigned long pulses = tachoPulseCount;
    unsigned long pulses2 = tacho2PulseCount;
    tachoPulseCount = 0;
    tacho2PulseCount = 0;
    interrupts();

    if (FAN_TACHO_PULSES_PER_REV > 0 && elapsed > 0) {
      fanRPM = (pulses * 60000UL) / elapsed / FAN_TACHO_PULSES_PER_REV;
#if FAN2_ENABLED
      fan2RPM = (pulses2 * 60000UL) / elapsed / FAN_TACHO_PULSES_PER_REV;
#else
      fan2RPM = 0;
#endif
    } else {
      fanRPM = 0;
      fan2RPM = 0;
    }

    lastTachoSample = now;
  }

  bool shouldSpin = targetFanPercent >= FAN_TACHO_MIN_CHECK_PERCENT;

  if (!shouldSpin) {
    fanDemandSince = 0;
    tachoFault = false;
    fan2TachoFault = false;
    return;
  }

  if (fanDemandSince == 0) {
    fanDemandSince = now;
    tachoFault = false;
    fan2TachoFault = false;
    return;
  }

  if (now - fanDemandSince >= FAN_TACHO_FAULT_DELAY_MS) {
    tachoFault = fanRPM < FAN_TACHO_MIN_RPM;
#if FAN2_ENABLED
    fan2TachoFault = fan2RPM < FAN_TACHO_MIN_RPM;
#else
    fan2TachoFault = false;
#endif
  }
#endif
}

void fan_loop(float t, float h) {

  t = safeTemp(t);
  h = safeHum(h);

  if (sensors_hasFault()) {
    fanReasonName = "SENSOR SAFE";
    targetFanPercent = FAN_SENSOR_FAIL_PERCENT;
    int targetPWM = percentToPwm(targetFanPercent);

    rampFanToTarget(targetPWM);
    writeFanPwm();
    updateFanTacho();

    if (millis() - lastFanLog >= FAN_LOG_INTERVAL_MS) {
      lastFanLog = millis();
      Serial.print("Fan SAFE | sensor ");
      Serial.print(sensors_getStatusName());
      Serial.print(" | target ");
      Serial.print(targetFanPercent);
      Serial.print("% | current ");
      Serial.print(getFanPercent());
      Serial.print("% | rpm ");
      Serial.print(getFanRPM());
#if FAN2_ENABLED
      Serial.print("/");
      Serial.print(getFan2RPM());
#endif
      Serial.print(" | tach ");
      Serial.println(fan_getTachoStatusName());
    }
    return;
  }

  if (ui_isFanManual()) {
    int value = ui_getFanManualValue();
    fanReasonName = "MANUAL";
    targetFanPercent = value;
    currentPWM = percentToPwm(value);

    writeFanPwm();
    updateFanTacho();
    return;
  }

  float activeErrorT = t - getTargetTemp();
  float activeErrorH = h - getTargetHum();
  if (activeErrorT < 0) activeErrorT = 0;
  if (activeErrorH < 0) activeErrorH = 0;

  targetFanPercent = runtimeConfig_evaluateFanPercent(activeErrorT, activeErrorH);

  if (activeErrorT > 0 && activeErrorH > 0) {
    fanReasonName = "TEMP+HUM";
  } else if (activeErrorT > 0) {
    fanReasonName = "TEMP";
  } else if (activeErrorH > 0) {
    fanReasonName = "HUM";
  } else {
    fanReasonName = "IDLE";
  }

  targetFanPercent = clamp(targetFanPercent, 0, FAN_MAX_PERCENT);
  int targetPWM = percentToPwm(targetFanPercent);

  rampFanToTarget(targetPWM);
  writeFanPwm();
  updateFanTacho();

  if (millis() - lastFanLog >= FAN_LOG_INTERVAL_MS) {
    lastFanLog = millis();
    Serial.print("Fan AUTO | T ");
    Serial.print(t, 1);
    Serial.print("/");
    Serial.print(getTargetTemp(), 1);
    Serial.print("C | H ");
    Serial.print(h, 0);
    Serial.print("/");
    Serial.print(getTargetHum(), 0);
    Serial.print("% | target ");
    Serial.print(targetFanPercent);
    Serial.print("% | current ");
    Serial.print(getFanPercent());
    Serial.print("% | rpm ");
    Serial.print(getFanRPM());
#if FAN2_ENABLED
    Serial.print("/");
    Serial.print(getFan2RPM());
#endif
    Serial.print(" | tach ");
    Serial.println(fan_getTachoStatusName());
  }
}
