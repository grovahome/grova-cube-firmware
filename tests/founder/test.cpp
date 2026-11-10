#include <Wire.h>

#include <cassert>
#include <iostream>
#include <limits>

#include "founder/i2c_bus.h"
#include "founder/output_policy.h"
#include "founder/outputs.h"
#include "founder/rtc.h"
#include "founder/rtc_codec.h"
#include "modules/signal_registry.h"
static unsigned long nowMs = 500;
unsigned long millis() {
  return nowMs;
}
static int pins[49] = {}, pwm[4] = {};
void digitalWrite(int pin, int v) {
  assert(pin >= 0 && pin < 49);
  pins[pin] = v;
}
void pinMode(int, int) {}
double ledcSetup(int, double f, int) {
  return f;
}
void ledcAttachPin(int, int) {}
void ledcWrite(int ch, int v) {
  assert(ch >= 2 && ch <= 3);
  pwm[ch] = v;
}
void neopixelWrite(int, int, int, int) {}
int main() {
  // Invalid dates, BCD and loss-of-time flags must never seed the system clock.
  uint8_t date[] = {0, 0, 0x12, 4, 0x29, 0x02, 0x24};
  uint32_t stamp = 0;
  assert(rv3032::decode(date, 0, 0, stamp));
  assert(stamp == 1709208000);
  for (uint8_t flag : {uint8_t(1), uint8_t(2), uint8_t(3)}) {
    assert(!rv3032::decode(date, flag, 0, stamp));
  }
  assert(!rv3032::decode(date, 0, 1, stamp));
  date[6] = 0x25;
  assert(!rv3032::decode(date, 0, 0, stamp));
  date[6] = 0x24;
  date[1] = 0x6a;
  assert(!rv3032::decode(date, 0, 0, stamp));
  date[1] = 0;
  date[3] = 7;
  assert(!rv3032::decode(date, 0, 0, stamp));
  date[3] = 4;
  for (uint32_t epoch : {1704067200UL, 1709208000UL, 1735689599UL, 4102444799UL}) {
    uint8_t encoded[7];
    uint32_t back = 0;
    assert(rv3032::encode(epoch, encoded));
    assert(rv3032::decode(encoded, 0, 0, back) && back == epoch);
  }
  uint8_t encoded[7];
  assert(!rv3032::encode(0, encoded));
  assert(!rv3032::encode(4102444800UL, encoded));
  // Bus scopes select exactly one channel and restore isolation, including failures.
  founderBus::begin();
  assert(founderBus::hasMux() && Wire.mask == 0);
  {
    founderBus::Scope channel(3);
    assert(channel && Wire.mask == 8);
    founderBus::Scope competing(1);
    assert(!competing);
  }
  assert(Wire.mask == 0);
  {
    founderBus::Scope invalid(8);
    assert(!invalid);
  }
  Wire.failMask = true;
  {
    founderBus::Scope broken(0);
    assert(!broken);
  }
  Wire.failMask = false;
  Wire.mux = false;
  founderBus::refresh();
  {
    founderBus::Scope direct;
    assert(direct);
  }
  {
    founderBus::Scope downstream(0);
    assert(!downstream);
  }
  Wire.mux = true;
  founderBus::refresh();
  // Actual production RTC driver configures backup with charging disabled.
  Wire.memory[0xC0] = 0x0F;
  assert(founderRtc::begin());
  assert(Wire.memory[0xC0] == 0x20);
  assert(Wire.memory[0x10] & 4);
  Wire.memory[0x0D] = 3;
  assert(!founderRtc::read(stamp));
  assert(founderRtc::write(1709208000));
  assert(founderRtc::read(stamp) && stamp == 1709208000);
  Wire.failRegister = 0x01;
  assert(!founderRtc::write(1735689599));
  assert(Wire.memory[0x11] & 1);
  assert(!founderRtc::read(stamp));
  Wire.failRegister = -1;
  assert(founderRtc::write(1709208000));
  Wire.rtc = false;
  assert(!founderRtc::read(stamp));
  assert(!founderRtc::present());
  Wire.rtc = true;
  // Drive the real generic output service: reboot/setup/Rest/OTA never replay output.
  founderOutputs::preinit();
  founderOutputs::begin();
  assert(!founderOutputs::set("OUT12_1", 100));
  founderOutputs::interlock(false, false);
  assert(founderOutputs::set("OUT12_1", 100) && pins[4] == 1);
  assert(!founderOutputs::set("OUT5_1", 50));
  assert(!founderOutputs::set("PUMP", 100));
  assert(!founderOutputs::set("0_10V_OUT_1", std::numeric_limits<float>::quiet_NaN()));
  assert(!founderOutputs::set("0_10V_OUT_1", 101));
  assert(founderOutputs::set("0_10V_OUT_1", 50) && pwm[2] == 2048);
  founderOutputs::interlock(true, false);
  assert(pins[4] == 0 && pwm[2] == 0);
  assert(!founderOutputs::set("OUT12_1", 100));
  assert(founderOutputs::set("OUT12_1", 0));
  founderOutputs::interlock(false, true);
  assert(!founderOutputs::set("OUT12_1", 100));
  founderOutputs::interlock(false, false);
  assert(pins[4] == 0 && pwm[2] == 0);
  assert(founderOutputs::set("0_10V_OUT_2", 100) && pwm[3] == 4095);
  founderOutputs::inhibit();
  assert(pwm[3] == 0);
  founderOutputs::interlock(false, false);
  assert(!founderOutputs::set("OUT12_2", 100));
  // Sample timestamps must survive publishing; non-finite data invalidates signals.
  signalRegistry_begin();
  signalRegistry_publishAt(SIGNAL_CLIMATE_TEMPERATURE_C, 23, "sensor", 100);
  assert(signalRegistry_ageMs(SIGNAL_CLIMATE_TEMPERATURE_C) == 400);
  nowMs = 1000;
  assert(signalRegistry_ageMs(SIGNAL_CLIMATE_TEMPERATURE_C) == 900);
  signalRegistry_publish(SIGNAL_CLIMATE_TEMPERATURE_C, INFINITY, "bad");
  assert(!signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C).valid);
  std::cout << "Founder calendar, bus isolation, RTC failure, output interlock and sample "
               "freshness checks passed\n";
}
