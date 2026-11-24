#include <Wire.h>
#include <driver/timer.h>

#include <cassert>
#include <iostream>
#include <limits>

#include "device_identity.h"
#include "drivers/ssd1317.h"
#include "drivers/ssd1317_protocol.h"
#include "controller/i2c_bus.h"
#include "controller/output_policy.h"
#include "controller/outputs.h"
#include "controller/rtc.h"
#include "controller/rtc_codec.h"
#include "modules/signal_registry.h"
static unsigned long nowMs = 500;
void delay(unsigned long ms) {
  nowMs += ms;
}
unsigned long millis() {
  return nowMs;
}
static int pins[49] = {}, pwm[4] = {};
static bool failPwm = false;
void digitalWrite(int pin, int v) {
  assert(pin >= 0 && pin < 49);
  pins[pin] = v;
}
void pinMode(int, int) {}
double ledcSetup(int, double f, int) {
  return failPwm ? 0 : f;
}
void ledcAttachPin(int, int) {}
void ledcWrite(int ch, int v) {
  assert(ch >= 2 && ch <= 3);
  pwm[ch] = v;
}
void neopixelWrite(int, int, int, int) {}
int main(int argc, char**) {
  if (argc > 1) {
    testTimer::fail = true;
    founderOutputs::preinit();
    founderOutputs::begin();
    assert(!founderOutputs::ready());
    founderOutputs::interlock(false, false);
    assert(!founderOutputs::set("OUT12_1", 100));
    return 0;
  }
  // Same manufacturer bytes must not collide across boards.
  char first[13], second[13];
  grovaMacSuffix(0x112233AABBCCULL, first);
  grovaMacSuffix(0x445566AABBCCULL, second);
  assert(std::string(first) == "CCBBAA332211" && std::string(first) != second);
  uint8_t init[32], canvas[ssd1317::BufferSize] = {};
  size_t length = ssd1317::initCommands(init, 80);
  assert(length <= 32 && init[0] == 0xFD && init[1] == 0x12 && init[8] == 80);
  canvas[0] = 0x80;
  canvas[7 * 16 + 15] = 1;
  canvas[8 * 16] = 0x80;
  assert(ssd1317::pageByte(canvas, 0, 0) == 1);
  assert(ssd1317::pageByte(canvas, 127, 0) == 128);
  assert(ssd1317::pageByte(canvas, 0, 1) == 1);
  assert(ssd1317::pageByte(canvas, 1, 0) == 0);
  assert(ssd1317::portraitPageByte(canvas, 63, 0) == 1);
  assert(ssd1317::portraitPageByte(canvas, 56, 15) == 128);
  assert(ssd1317::portraitPageByte(canvas, 55, 0) == 1);
  assert(ssd1317::portraitPageByte(canvas, 0, 15) == 0);
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
  assert(!founderOutputs::startTest(0, 50, 10000));
  assert(!founderOutputs::startTest(0, 100, 10001));
  const int8_t assignedOwners[5] = {0, -1, -1, -1, -1};
  const bool untimed[5] = {};
  founderOutputs::assignOwners(assignedOwners, untimed);
  assert(!founderOutputs::startTest(0, 100, 10000));
  assert(founderOutputs::startTest(1, 100, 10000));
  founderOutputs::interlock(true, false);
  assert(pins[5] == 1 && pins[4] == 0);
  nowMs += 10000;
  testTimer::fire();
  assert(pins[5] == 0);
  nowMs -= 10000;
  founderOutputs::stopTest();
  assert(founderOutputs::startTest(3, 50, 10000));
  founderOutputs::interlock(true, false);
  assert(pwm[2] == 2048 && pwm[3] == 0);
  assert(founderOutputs::setTestLevel(100) && pwm[2] == 4095);
  founderOutputs::interlock(false, true);
  assert(!founderOutputs::testing() && pwm[2] == 0);
  assert(!founderOutputs::startTest(3, 50, 10000));
  assert(!founderOutputs::set("OUT12_1", 100));
  founderOutputs::interlock(false, false);
  assert(pins[4] == 0 && pwm[2] == 0);
  assert(founderOutputs::set("0_10V_OUT_2", 100) && pwm[3] == 4095);
  founderOutputs::inhibit();
  assert(pwm[3] == 0);
  founderOutputs::interlock(false, false);
  assert(!founderOutputs::set("OUT12_2", 100));
  failPwm = true;
  founderOutputs::preinit();
  founderOutputs::begin();
  assert(!founderOutputs::ready());
  founderOutputs::interlock(false, false);
  assert(!founderOutputs::set("OUT12_1", 100));
  failPwm = false;
  // Sample timestamps must survive publishing; non-finite data invalidates signals.
  signalRegistry_begin();
  signalRegistry_publishAt(SIGNAL_CLIMATE_TEMPERATURE_C, 23, "sensor", 100);
  assert(signalRegistry_ageMs(SIGNAL_CLIMATE_TEMPERATURE_C) == 400);
  nowMs = 1000;
  assert(signalRegistry_ageMs(SIGNAL_CLIMATE_TEMPERATURE_C) == 900);
  signalRegistry_publish(SIGNAL_CLIMATE_TEMPERATURE_C, INFINITY, "bad");
  assert(!signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C).valid);
  GrovaSSD1317 oled;
  assert(oled.begin(0x3C, 80));
  assert(Wire.oledBytes == 1024);
  assert(!oled.begin(0x3C, 96));
  Wire.failOledData = true;
  assert(!oled.display());
  Wire.failOledData = false;
  assert(oled.display());
  Wire.oled = false;
  assert(!oled.begin(0x3C, 80));
  std::cout << "Founder calendar, bus isolation, RTC failure, output interlock and sample "
               "freshness checks passed\n";
}
