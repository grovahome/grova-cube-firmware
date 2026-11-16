#include "controller/outputs.h"

#include "boards/grova_core_s3.h"
#include "controller/output_policy.h"
#include "controller/timed_digital.h"
namespace founderOutputs {
static Policy policy;
static const int pins[] = {founder::OUTPUTS[0].pin, founder::OUTPUTS[1].pin,
                           founder::OUTPUTS[2].pin, founder::OUTPUTS[3].pin,
                           founder::OUTPUTS[4].pin};
static const char* names[] = {founder::OUTPUTS[0].id, founder::OUTPUTS[1].id,
                              founder::OUTPUTS[2].id, founder::OUTPUTS[3].id,
                              founder::OUTPUTS[4].id};
static float values[5] = {};
static bool pwmReady[2] = {};
static int8_t owners[5] = {-1, -1, -1, -1, -1};
static bool timed[5] = {};
static void write(int i, float value) {
  values[i] = value;
  if (i < 3) {
    founderDigital::set(i, value > 0);
  } else if (pwmReady[i - 3]) {
    ledcWrite(i - 1, lroundf(value * 4095.0f / 100.0f));  // channels 2,3; fans own 0,1.
  }
}
static void off() {
  for (int i = 0; i < 5; ++i) {
    write(i, 0);
  }
}
void preinit() {
  policy = Policy{};
  for (int i = 0; i < 5; ++i) {
    values[i] = 0;
  }
  pwmReady[0] = pwmReady[1] = false;
  for (int pin : pins) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  digitalWrite(founder::RS485_DE_RE, LOW);
  pinMode(founder::RS485_DE_RE, OUTPUT);
  neopixelWrite(founder::RGB_DATA, 0, 0, 0);
}
void begin() {
  for (int i = 0; i < 2; ++i) {
    pwmReady[i] = ledcSetup(i + 2, 5000, 12) > 0;
    if (pwmReady[i]) {
      ledcAttachPin(pins[i + 3], i + 2);
      ledcWrite(i + 2, 0);
    }
  }
  policy.ready = founderDigital::begin() && pwmReady[0] && pwmReady[1];
  off();
}
void interlock(bool rest, bool setup) {
  policy.rest = rest;
  policy.setup = setup;
  if (rest || setup || policy.ota) {
    off();
  }
}
void stopAll() {
  off();
}
void inhibit() {
  policy.ota = true;
  off();
}
bool ready() {
  return policy.ready;
}
bool inhibited() {
  return policy.ota;
}
bool set(const char* name, float value) {
  for (int i = 0; i < 5; ++i) {
    if (strcmp(name, names[i]) == 0) {
      if (owners[i] >= 0) {
        return false;
      }
      if (!policy.allows(value) || (i < 3 && value != 0 && value != 100)) {
        return false;
      }
      write(i, value);
      return true;
    }
  }
  return false;
}
void assignOwners(const int8_t (&next)[5], const bool (&pulse)[5]) {
  off();
  for (int i = 0; i < 5; ++i) {
    owners[i] = next[i];
    timed[i] = pulse[i];
  }
}
bool setDevice(unsigned owner, unsigned channel, float percent) {
  if (channel >= 5 || owner >= 5 || owners[channel] != int(owner) || !policy.allows(percent)) {
    return false;
  }
  if (channel < 3 && percent != 0 && percent != 100) {
    return false;
  }
  if (timed[channel] && percent > 0) {
    return false;
  }
  write(channel, percent);
  return true;
}
bool pulseDevice(unsigned owner, unsigned channel, unsigned durationMs) {
  if (channel >= 3 || owner >= 5 || owners[channel] != int(owner) || !timed[channel] ||
      !policy.allows(100)) {
    return false;
  }
  if (!founderDigital::pulse(channel, durationMs)) {
    return false;
  }
  values[channel] = 100;
  return true;
}
float applied(unsigned channel) {
  if (channel >= 5) {
    return 0;
  }
  return channel < 3 ? (founderDigital::on(channel) ? 100 : 0) : values[channel];
}
unsigned remainingMs(unsigned channel) {
  return channel < 3 ? founderDigital::remainingMs(channel) : 0;
}
void appendJson(String& j) {
  j += "\"outputs\":[";
  for (int i = 0; i < 5; ++i) {
    if (i) {
      j += ',';
    }
    j += "{\"id\":\"";
    j += names[i];
    j += "\",\"kind\":\"";
    j += i < 3 ? "switch" : "0_10v";
    j += "\",\"percent\":";
    j += String(applied(i), 2);
    j += " ,\"voltage_setpoint\":";
    if (i < 3) {
      j += "null";
    } else {
      j += String(values[i] / 10.0f, 2);
    }
    j += " ,\"assigned\":";
    j += owners[i] >= 0 ? "true" : "false";
    j += " ,\"device_slot\":";
    if (owners[i] >= 0) {
      j += owners[i];
    } else {
      j += "null";
    }
    j += " ,\"ready\":";
    j += policy.ready ? "true" : "false";
    j += '}';
  }
  j += ']';
}
}  // namespace founderOutputs
