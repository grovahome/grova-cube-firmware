#include "founder/app.h"

#include <Preferences.h>

#include <cassert>
#include <iostream>

#include "founder/network.h"
#include "founder/outputs.h"
#include "modules/fan.h"
#include "modules/fan_policy.h"
#include "modules/rest_mode.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
static bool setup = false, testing = false, clockReady = true;
static unsigned long nowMs = 1000;
unsigned long millis() {
  return nowMs;
}
void digitalWrite(int, int) {}
void pinMode(int, int) {}
double ledcSetup(int, double f, int) {
  return f;
}
void ledcAttachPin(int, int) {}
void ledcWrite(int, int) {}
void neopixelWrite(int, int, int, int) {}
extern const char GROVA_BUILD_DATE[] = "test", GROVA_BUILD_TIME[] = "test";
bool wifiOTA_requiresSetup() {
  return setup;
}
bool wifiOTA_isTesting() {
  return testing;
}
bool wifiOTA_isConnected() {
  return true;
}
const char* wifiOTA_getIP() {
  return "127.0.0.1";
}
bool wifiOTA_storageReady() {
  return true;
}
bool time_settingsReady() {
  return true;
}
bool isTimeSynced() {
  return clockReady;
}
unsigned long getEpochSeconds() {
  return 1800000000;
}
bool rtc_isEnabled() {
  return true;
}
bool rtc_isPresent() {
  return true;
}
bool rtc_hasValidTime() {
  return true;
}
bool rtc_wasUsedForBoot() {
  return false;
}
bool rtc_lastWriteOk() {
  return true;
}
bool rtc_setEnabled(bool) {
  return true;
}
const char* time_getSourceName() {
  return "ntp";
}
namespace founderRtc {
bool backupConfigured() {
  return true;
}
}  // namespace founderRtc
namespace founderBus {
bool hasMux() {
  return true;
}
}  // namespace founderBus
namespace founderDevices {
void appendJson(String& j) {
  j += "\"devices\":[]";
}
}  // namespace founderDevices
namespace founderNetwork {
bool connected() {
  return false;
}
bool configure(JsonObjectConst) {
  return true;
}
void appendConfig(JsonObject j) {
  j["host"] = "";
}
}  // namespace founderNetwork
int getFanPercent() {
  return 0;
}
int getFan2Percent() {
  return 0;
}
int getFanTargetPercent() {
  return 0;
}
int getFan2TargetPercent() {
  return 0;
}
int getFanRPM() {
  return 0;
}
int getFan2RPM() {
  return 0;
}
bool fan_hasTachoFault() {
  return false;
}
bool fan_getTachoFault(int) {
  return false;
}
const char* fan_getModeName(int) {
  return "REST";
}
const char* fan_getReasonName(int) {
  return "REST";
}
void fan_forceOff() {}
bool fanPolicy_settingsReady() {
  return true;
}
bool fan_setManual(int, int, bool) {
  return true;
}
bool fan_setAuto(int, bool) {
  return true;
}
bool fanPolicy_applyJson(const String&, String&) {
  return false;
}
int fanPolicy_extractFan(const String&) {
  return 1;
}
bool fan_applyPolicyConfig(int) {
  return true;
}
void fanControl_appendJson(String& j) {
  j += "\"fan_control\":{}";
}
extern float getTargetTemp();
static bool run(const char* body, bool remote = false) {
  String response;
  return founderApp::command(body, response, remote);
}
int main() {
  restMode_begin();
  founderApp::settingsBegin();
  founderOutputs::preinit();
  founderOutputs::begin();
  signalRegistry_begin();
  assert(founderApp::setRest(true));
  assert(!run(R"({"cmd":"set_output","output":"OUT12_1","state":true})"));
  assert(!run(R"({"cmd":"set_rest_mode","enabled":"false"})"));
  assert(!run(R"({"cmd":"set_grow_mode","mode":"GROWTH"})"));
  assert(!run(R"({"cmd":"pump_test","action":"start"})"));
  assert(run(R"({"cmd":"set_rest_mode","enabled":false})"));
  assert(run(R"({"cmd":"set_output","output":"OUT12_1","state":true})"));
  assert(!run(R"({"cmd":"set_output","output":"OUT12_1","percent":50})"));
  assert(!run(R"({"cmd":"set_output","output":"0_10V_OUT_1","percent":"100"})"));
  assert(run(R"({"cmd":"set_output","output":"0_10V_OUT_1","percent":42.5})"));
  testing = true;
  assert(!run(R"({"cmd":"set_output","output":"OUT12_1","state":true})"));
  testing = false;
  setup = true;
  assert(!run(R"({"cmd":"set_rest_mode","enabled":false})"));
  setup = false;
  assert(!run(R"({"cmd":"set_output","output":"OUT12_1","state":true})", true));
  assert(!run(
      R"({"cmd":"set_output","output":"OUT12_1","state":true,"cmd_id":"expired","expires_at_s":1800000000})",
      true));
  clockReady = false;
  assert(!run(
      R"({"cmd":"set_output","output":"OUT12_1","state":true,"cmd_id":"clock","expires_at_s":1800000010})",
      true));
  clockReady = true;
  const char* packet =
      R"({"cmd":"set_output","output":"OUT12_1","state":true,"cmd_id":"valid","expires_at_s":1800000010})";
  assert(run(packet, true));
  assert(!run(packet, true));
  assert(founderApp::setRest(true));
  float previous = getTargetTemp();
  testNvs::failWrites = true;
  assert(!run(R"({"cmd":"set_targets","temperature_c":30,"humidity_pct":60})"));
  assert(getTargetTemp() == previous);
  assert(!founderApp::setRest(false));
  testNvs::failWrites = false;
  assert(run(R"({"cmd":"set_targets","temperature_c":30,"humidity_pct":60})"));
  assert(getTargetTemp() == 30);
  signalRegistry_publish(SIGNAL_CLIMATE_TEMPERATURE_C, 22, "test");
  JsonDocument status;
  String body = founderApp::status();
  assert(!deserializeJson(status, body));
  assert(status["signals"].size() == 6 && status["outputs"].size() == 5);
  assert(status["signals"][0]["value"].as<float>() == 22);
  assert(status["signals"][1]["value"].isNull());
  assert(status["environment"]["temperature_c"].as<float>() == 22);
  assert(!status["automation_supported"].as<bool>());
  founderOutputs::inhibit();
  assert(!run(R"({"cmd":"set_rest_mode","enabled":false})"));
  std::cout << "Founder production command parsing, deadlines, duplicate suppression, storage "
               "rollback and status JSON passed\n";
}
