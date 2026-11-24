#include "controller/display.h"

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Wire.h>

#include <cassert>
#include <iostream>

#include "controller/boot_animation.h"
#include "controller/equipment.h"
#include "controller/i2c_bus.h"
#include "controller/output_test.h"
#include "modules/signal_registry.h"
#include "controller/outputs.h"
static unsigned long now = 100;
unsigned long millis() {
  return now;
}
void delay(unsigned long ms) {
  now += ms;
}
static bool paused = true;
namespace founderApp {
bool setRest(bool enabled) {
  paused = enabled;
  return true;
}
const char* deviceId() {
  return "grova-test";
}
}  // namespace founderApp
bool restMode_isEnabled() {
  return paused;
}
static bool setupRequired = false, wifiTesting = false;
bool wifiOTA_requiresSetup() {
  return setupRequired;
}
bool wifiOTA_isTesting() {
  return wifiTesting;
}
namespace founderEquipment {
static uint32_t configRevision = 1;
static unsigned lightLevel = 0, overrideSeconds = 0;
bool settingsReady() {
  return true;
}
uint32_t localRevision() {
  return configRevision;
}
bool localDevice(unsigned index, LocalDevice& device) {
  if (index != 0) return false;
  device = {"Grow Light", LocalMode::Scheduled, false, false, lightLevel, overrideSeconds};
  return true;
}
bool setLocalLevel(unsigned index, unsigned percent) {
  if (index != 0 || paused) return false;
  lightLevel = percent;
  overrideSeconds = 900;
  return true;
}
bool stopLocalTimed(unsigned) {
  return false;
}
bool cancelLocalOverride(unsigned) {
  overrideSeconds = 0;
  return true;
}
NextAction nextAction() {
  NextAction action;
  if (!paused) {
    action.name = "Grow Light";
    action.minute = 720;
  }
  return action;
}
}  // namespace founderEquipment
static unsigned simulatedPort = 7, simulatedPercent = 0;
static bool portAssigned = false;
bool fan_startTest(int fan, int percent) {
  simulatedPort = 4 + fan;
  simulatedPercent = percent;
  return true;
}
bool fan_testing() {
  return simulatedPort == 5 || simulatedPort == 6;
}
bool fan_setTestLevel(int percent) {
  assert(fan_testing());
  simulatedPercent = percent;
  return true;
}
void fan_stopTest() {
  simulatedPort = 7;
  simulatedPercent = 0;
}
static bool setupActive = true;
bool wifiOTA_setupActive() {
  return setupActive;
}
bool isTimeSynced() {
  return false;
}
namespace founderOutputs {
float applied(unsigned channel) {
  return channel == simulatedPort ? simulatedPercent : 0;
}
bool assigned(unsigned channel) {
  return channel == 0 && portAssigned;
}
bool ready() {
  return true;
}
bool inhibited() {
  return false;
}
bool startTest(unsigned channel, unsigned percent, unsigned duration) {
  assert(duration == founderOutputTest::DurationMs);
  assert(!assigned(channel));
  simulatedPort = channel;
  simulatedPercent = percent;
  return true;
}
bool testing() {
  return simulatedPort < 5;
}
bool setTestLevel(unsigned percent) {
  if (simulatedPort < 3 || simulatedPort >= 5) {
    return false;
  }
  simulatedPercent = percent;
  return true;
}
void stopTest() {
  simulatedPort = 7;
  simulatedPercent = 0;
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
  check(-2, true, "Output Test");   // Service.
  check(2, true, "0-10V 2");
  check(0, true, "Output Test");
  check(-2, true, "Turn/Press  Hold:Home");
  check(1, true, "Press: Menu");
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
  check(0, true, "Press: Menu");  // Dismiss the welcome.
  signalRegistry_publish(SIGNAL_CLIMATE_TEMPERATURE_C, 23, "SHT4x@ch3/0x44");
  check(0, true, "Climate");
  check(0, true, "SHT4x / Port 3");
  now += 4500;
  testGraphics::lines.clear();
  founderDisplay::loop();
  assert(testGraphics::lines.find("Last 4s") != std::string::npos);
  signalRegistry_invalidate(SIGNAL_CLIMATE_TEMPERATURE_C);
  now += 500;
  testGraphics::lines.clear();
  founderDisplay::loop();
  assert(testGraphics::lines.find("Offline 5s") != std::string::npos);
  founderDisplay::navigate(0, false, true);
  check(0, false, "Sensor offline");
  signalRegistry_publish(SIGNAL_CLIMATE_TEMPERATURE_C, 24, "SHT4x@ch3/0x44");
  now += 500;
  check(0, false, "Schedule paused");

  now += 60000;
  founderDisplay::loop();
  assert(!Wire.oledPowered);
  const auto sleepingBytes = Wire.oledBytes;
  now += 3000;
  founderDisplay::loop();
  assert(Wire.oledBytes == sleepingBytes);
  check(0, true, "Press: Menu");  // First input wakes without opening the menu.
  assert(Wire.oledPowered);
  check(0, true, "Climate");
  check(3, true, "Settings");
  check(1, true, "Idle after 1 min");
  check(1, true, "GROVA Saver");
  founderDisplay::begin();
  founderDisplay::loop();
  now += 60000;
  const auto beforeSaver = Wire.oledBytes;
  founderDisplay::loop();
  assert(Wire.oledPowered && Wire.oledBytes > beforeSaver);
  testGraphics::lines.clear();
  now += 500;
  founderDisplay::loop();
  assert(testGraphics::lines.empty());  // Wordmark is drawn as graphics.
  check(0, true, "Press: Menu");
  check(0, true, "Climate");
  check(3, true, "Settings");
  check(1, true, "Idle after 1 min");
  check(1, true, "Always On");
  now += 60000;
  check(0, false, "Always On");
  assert(Wire.oledPowered);
  founderDisplay::navigate(0, false, true);
  check(0, true, "Climate");
  check(4, true, "Service");
  check(1, true, "Output Test");
  check(1, true, "Press Back");
  portAssigned = true;
  check(1, false, "Assigned");
  check(0, true, "Assigned");
  assert(!founderOutputTest::running());
  portAssigned = false;
  paused = false;
  check(0, true, "Pause via Operation");
  assert(!founderOutputTest::running());
  paused = true;
  check(0, true, "Press Stop");
  assert(simulatedPort == 0 && simulatedPercent == 100);
  assert(!founderOutputTest::start(1, 100));
  check(0, true, "Press Back");
  assert(simulatedPort == 7);
  check(0, true, "Output Test");
  check(-2, true, "Fan 2");
  check(1, false, "Press Run");
  check(0, true, "Press Stop");
  assert(simulatedPort == 6 && simulatedPercent == 10);
  now += 2000;
  check(1, false, "Press Stop");
  assert(simulatedPercent == 20);
  assert(founderOutputTest::remainingMs() == 8000);
  now += 8000;
  check(0, false, "Press Run");
  assert(simulatedPort == 7);
  check(0, true, "Press Stop");
  wifiTesting = true;
  now += 500;
  check(0, false, "Test unavailable");
  assert(simulatedPort == 7);
  wifiTesting = false;
  founderDisplay::navigate(0, false, true);
  check(0, true, "Operation");
  check(1, true, "Resume");
  check(1, true, "Schedule Grow Light");
  assert(paused);
  ++founderEquipment::configRevision;
  check(3, true, "Operation");
  assert(paused);  // Confirm cannot release a different configuration.
  check(1, true, "Resume");
  check(3, true, "ACTIVE");
  assert(!paused);
  founderDisplay::navigate(0, false, true);
  check(0, false, "12:00 Off Grow Light");
  check(0, true, "Devices");
  check(2, true, "Grow Light");
  check(1, true, "OFF");
  check(1, false, "15 min");
  check(0, true, "Override 15:00 left");
  assert(founderEquipment::lightLevel == 100);
  check(0, true, "Schedule");
  assert(founderEquipment::overrideSeconds == 0);
  founderDisplay::navigate(0, false, true);
  check(0, true, "Operation");
  check(1, true, "Pause all");
  check(1, true, "All outputs off");
  assert(!paused);
  check(2, true, "PAUSED");
  assert(paused);
  founderDisplay::navigate(0, false, true);
  check(0, true, "Settings");
  check(3, true, "Brightness");
  check(2, true, "Medium");
  check(-2, true, "Low");
  assert(Wire.oledContrast == 0x13);  // First brightness preset in SH1107.
  founderDisplay::begin();
  founderDisplay::loop();
  assert(Wire.oledContrast == 0x13);  // Saved preset survives restart.
  std::cout << "OLED daily operation, configuration review, brightness and diagnostics passed\n";
}
