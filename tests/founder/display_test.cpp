#include "founder/display.h"

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Wire.h>

#include <cassert>
#include <iostream>

#include "founder/boot_animation.h"
#include "founder/i2c_bus.h"
static unsigned long now = 100;
unsigned long millis() {
  return now;
}
void delay(unsigned long ms) {
  now += ms;
}
namespace founderApp {
const char* deviceId() {
  return "grova-test";
}
}  // namespace founderApp
bool restMode_isEnabled() {
  return true;
}
bool wifiOTA_setupActive() {
  return true;
}
const char* wifiOTA_setupSSID() {
  return "GROVA-test";
}
const char* wifiOTA_setupPassword() {
  return "test-only";
}
const char* wifiOTA_setupIP() {
  return "192.168.4.1";
}
const char* wifiOTA_setupState() {
  return "AP_READY";
}
bool wifiOTA_isConnected() {
  return false;
}
const char* wifiOTA_getIP() {
  return "0.0.0.0";
}
int getFanPercent() {
  return 0;
}
int getFan2Percent() {
  return 0;
}
int getFanRPM() {
  return 0;
}
int getFan2RPM() {
  return 0;
}
const char* fan_getTachoStatusName() {
  return "OK";
}
bool rtc_hasValidTime() {
  return false;
}
const char* time_getSourceName() {
  return "NONE";
}
int getHour() {
  return 0;
}
int getMinute() {
  return 0;
}
int main() {
  founderBus::begin();
  founderDisplay::begin();
  founderDisplay::loop();
  assert(founderDisplay::present());
  assert(testGraphics::textCalls == 0);
  const auto bytes = Wire.oledBytes;
  now += 50;
  founderDisplay::loop();
  assert(Wire.oledBytes == bytes);
  now += 50;
  founderDisplay::navigate(0);
  founderDisplay::loop();
  assert(Wire.oledBytes > bytes && testGraphics::textCalls == 0);
  now += founderBoot::DurationMs;
  founderDisplay::loop();
  assert(testGraphics::lines.find("GROVA Wi-Fi Setup") != std::string::npos);
  testGraphics::textCalls = 0;
  Wire.oled = false;
  now += 500;
  founderDisplay::loop();
  assert(!founderDisplay::present());
  Wire.oled = true;
  now += 10000;
  founderDisplay::loop();
  assert(founderDisplay::present() && testGraphics::textCalls > 0);  // No replay after hot-plug.
  founderDisplay::begin();
  testGraphics::textCalls = 0;
  founderDisplay::loop();
  assert(testGraphics::textCalls == 0);
  founderDisplay::navigate(1);
  founderDisplay::loop();
  assert(testGraphics::textCalls > 0);  // User can skip immediately.
  assert(founderDisplay::configure(1, 80));
  testGraphics::textCalls = 0;
  now += 500;
  founderDisplay::loop();
  assert(testGraphics::textCalls > 0);  // Rotation setting must not replay welcome.
  std::cout << "Display service: welcome cadence, Wi-Fi handoff, user skip, recovery and rotation "
               "reconfiguration passed\n";
}
