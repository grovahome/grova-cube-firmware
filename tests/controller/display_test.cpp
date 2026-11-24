#include "controller/display.h"

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Wire.h>

#include <cassert>
#include <iostream>

#include "controller/boot_animation.h"
#include "controller/i2c_bus.h"
#include "controller/outputs.h"
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
static bool setupActive = true;
bool wifiOTA_setupActive() {
  return setupActive;
}
bool isTimeSynced() {
  return false;
}
namespace founderOutputs {
float applied(unsigned) {
  return 0;
}
}  // namespace founderOutputs
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
  founderDisplay::navigate(0, false);
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
  setupActive = false;
  founderDisplay::navigate(1, false);
  founderDisplay::loop();
  assert(testGraphics::lines.find("Press: Menu") != std::string::npos);
  auto check = [](int turn, bool pressed, const char* expected) {
    testGraphics::lines.clear();
    founderDisplay::navigate(turn, pressed);
    founderDisplay::loop();
    assert(testGraphics::lines.find(expected) != std::string::npos);
  };
  check(0, true, "Turn/Press  Hold:Home");
  check(0, true, "Temperature");
  check(0, true, "Turn/Press  Hold:Home");
  check(1, false, "Turn/Press  Hold:Home");
  check(0, true, "0-10V 2");
  check(0, true, "Turn/Press  Hold:Home");
  check(-1, false, "Turn/Press  Hold:Home");
  check(-1, false, "Turn/Press  Hold:Home");
  check(0, true, "Press: Menu");
  check(0, true, "Back to Home");
  check(0, true, "Temperature");
  testGraphics::lines.clear();
  founderDisplay::navigate(0, false, true);
  founderDisplay::loop();
  assert(testGraphics::lines.find("Press: Menu") != std::string::npos);
  assert(founderDisplay::configure(1, 80));
  testGraphics::textCalls = 0;
  now += 500;
  founderDisplay::loop();
  assert(testGraphics::textCalls > 0);  // Rotation setting must not replay welcome.
  assert(!founderDisplay::selectController("unknown"));
  assert(founderDisplay::selectController("ssd1306"));
  assert(std::string(founderDisplay::controller()) == "SSD1306");
  assert(founderDisplay::offset() == 0);
  assert(!founderDisplay::configure(0, 80));
  founderDisplay::loop();
  assert(founderDisplay::present());
  founderDisplay::begin();
  assert(std::string(founderDisplay::controller()) == "SSD1306");
  assert(founderDisplay::rotation() == 1 && founderDisplay::offset() == 0);
  assert(founderDisplay::selectController("ssd1317"));
  assert(founderDisplay::offset() == 80);
  assert(founderDisplay::configure(2, 0, "ssd1312"));
  assert(std::string(founderDisplay::controller()) == "SSD1312");
  assert(!founderDisplay::configure(2, 64, "ssd1312"));
  founderDisplay::begin();
  assert(std::string(founderDisplay::controller()) == "SSD1312");
  assert(founderDisplay::rotation() == 2 && founderDisplay::offset() == 0);
  founderDisplay::loop();
  assert(founderDisplay::present());
  assert(founderDisplay::selectController("sh1107"));
  assert(founderDisplay::offset() == 96);
  assert(!founderDisplay::configure(2, 128));
  founderDisplay::begin();
  assert(std::string(founderDisplay::controller()) == "SH1107");
  assert(founderDisplay::rotation() == 2 && founderDisplay::offset() == 96);
  founderDisplay::loop();
  assert(founderDisplay::present());
  std::cout << "Display service: welcome cadence, Wi-Fi handoff, user skip, recovery and rotation "
               "reconfiguration passed\n";
}
