#include "founder/outputs.h"

#include "boards/grova_founder_s3.h"
#include "founder/output_policy.h"
namespace founderOutputs {
static Policy policy;
static const int pins[] = {founder::OUT12_1, founder::OUT5_1, founder::OUT12_2,
                           founder::ANALOG_OUT_1, founder::ANALOG_OUT_2};
static const char* names[] = {"OUT12_1", "OUT5_1", "OUT12_2", "0_10V_OUT_1", "0_10V_OUT_2"};
static float values[5] = {};
static bool pwmReady[2] = {};
static void write(int i, float value) {
  values[i] = value;
  if (i < 3) {
    digitalWrite(pins[i], value > 0 ? HIGH : LOW);
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
  policy.ready = pwmReady[0] && pwmReady[1];
  off();
}
void interlock(bool rest, bool setup) {
  policy.rest = rest;
  policy.setup = setup;
  if (rest || setup || policy.ota) {
    off();
  }
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
      if (!policy.allows(value) || (i < 3 && value != 0 && value != 100)) {
        return false;
      }
      write(i, value);
      return true;
    }
  }
  return false;
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
    j += String(values[i], 2);
    j += " ,\"voltage_setpoint\":";
    if (i < 3) {
      j += "null";
    } else {
      j += String(values[i] / 10.0f, 2);
    }
    j += " ,\"ready\":";
    j += policy.ready ? "true" : "false";
    j += '}';
  }
  j += ']';
}
}  // namespace founderOutputs
