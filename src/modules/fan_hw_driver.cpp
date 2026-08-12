#include <Arduino.h>
#include "config.h"
#include "modules/fan_hw_driver.h"

static constexpr int FAN_PWM_CHANNEL_1 = 0;
static constexpr int FAN_PWM_CHANNEL_2 = 1;
static constexpr int FAN_PWM_FREQUENCY = 25000;
static constexpr int FAN_PWM_RESOLUTION = 8;

struct FanHardwareChannel {
  int pwm = 0;
  int rpm = 0;
};

static FanHardwareChannel channels[2];
static volatile unsigned long tachoPulseCount1 = 0;
static volatile unsigned long tachoPulseCount2 = 0;
static unsigned long lastTachoSample = 0;

static void IRAM_ATTR onFan1TachoPulse() { tachoPulseCount1++; }
static void IRAM_ATTR onFan2TachoPulse() { tachoPulseCount2++; }

static int indexFor(int fan) { return fan == 2 ? 1 : 0; }
static int pwmChannelFor(int fan) { return fan == 2 ? FAN_PWM_CHANNEL_2 : FAN_PWM_CHANNEL_1; }

bool fanHw_isEnabled(int fan) {
  if (fan == 1) return true;
  if (fan == 2) return FAN2_ENABLED;
  return false;
}

bool fanHw_isTachoEnabled(int fan) {
  if (fan == 1) return FAN_TACHO_ENABLED;
  if (fan == 2) return FAN2_ENABLED && FAN2_TACHO_ENABLED;
  return false;
}

void fanHw_preinit() {
  pinMode(FAN_PWM, OUTPUT);
  digitalWrite(FAN_PWM, LOW);
#if FAN2_ENABLED
  pinMode(FAN2_PWM, OUTPUT);
  digitalWrite(FAN2_PWM, LOW);
#endif
  channels[0] = FanHardwareChannel{};
  channels[1] = FanHardwareChannel{};
}

void fanHw_begin() {
  fanHw_preinit();
  ledcSetup(FAN_PWM_CHANNEL_1, FAN_PWM_FREQUENCY, FAN_PWM_RESOLUTION);
  ledcAttachPin(FAN_PWM, FAN_PWM_CHANNEL_1);
#if FAN2_ENABLED
  ledcSetup(FAN_PWM_CHANNEL_2, FAN_PWM_FREQUENCY, FAN_PWM_RESOLUTION);
  ledcAttachPin(FAN2_PWM, FAN_PWM_CHANNEL_2);
#endif
  fanHw_emergencyStop(1);
  fanHw_emergencyStop(2);

#if FAN_TACHO_ENABLED
  pinMode(FAN_TACHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(FAN_TACHO), onFan1TachoPulse, FALLING);
#endif
#if FAN2_ENABLED && FAN2_TACHO_ENABLED
  pinMode(FAN2_TACHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(FAN2_TACHO), onFan2TachoPulse, FALLING);
#endif
  lastTachoSample = millis();
}

void fanHw_writePwm(int fan, int pwm) {
  if (!fanHw_isEnabled(fan)) return;
  pwm = constrain(pwm, 0, 255);
  channels[indexFor(fan)].pwm = pwm;
  ledcWrite(pwmChannelFor(fan), pwm);
}

void fanHw_emergencyStop(int fan) {
  if (!fanHw_isEnabled(fan)) return;
  channels[indexFor(fan)].pwm = 0;
  ledcWrite(pwmChannelFor(fan), 0);
}

int fanHw_getPwm(int fan) {
  return fanHw_isEnabled(fan) ? channels[indexFor(fan)].pwm : 0;
}

int fanHw_getRpm(int fan) {
  return fanHw_isEnabled(fan) ? channels[indexFor(fan)].rpm : 0;
}

void fanHw_update() {
#if !FAN_TACHO_ENABLED && !(FAN2_ENABLED && FAN2_TACHO_ENABLED)
  channels[0].rpm = 0;
  channels[1].rpm = 0;
  return;
#else
  const unsigned long now = millis();
  const unsigned long elapsed = now - lastTachoSample;
  if (elapsed < FAN_TACHO_SAMPLE_MS) return;

  noInterrupts();
  const unsigned long pulses1 = tachoPulseCount1;
  const unsigned long pulses2 = tachoPulseCount2;
  tachoPulseCount1 = 0;
  tachoPulseCount2 = 0;
  interrupts();

  if (FAN_TACHO_PULSES_PER_REV > 0 && elapsed > 0) {
    channels[0].rpm = fanHw_isTachoEnabled(1)
      ? (pulses1 * 60000UL) / elapsed / FAN_TACHO_PULSES_PER_REV
      : 0;
    channels[1].rpm = fanHw_isTachoEnabled(2)
      ? (pulses2 * 60000UL) / elapsed / FAN_TACHO_PULSES_PER_REV
      : 0;
  } else {
    channels[0].rpm = 0;
    channels[1].rpm = 0;
  }
  lastTachoSample = now;
#endif
}

