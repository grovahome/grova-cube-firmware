#include "founder/app.h"

#include <Preferences.h>
#include <driver/timer.h>

#include <cassert>
#include <iostream>

#include "founder/equipment.h"
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
static int gpio[49] = {};
void digitalWrite(int pin, int value) {
  gpio[pin] = value;
}
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
static int manualFans[2] = {};
static bool failFanSave = false;
bool fan_setManual(int fan, int percent, bool) {
  manualFans[fan - 1] = percent;
  return !failFanSave;
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
int main(int argc, char**) {
  restMode_begin();
  founderApp::settingsBegin();
  founderOutputs::preinit();
  founderOutputs::begin();
  founderEquipment::begin();
  signalRegistry_begin();
  assert(founderApp::setRest(true));
  assert(!run(R"({"cmd":"test_output","output":"OUT12_1","state":true})"));
  assert(!run(R"({"cmd":"set_rest_mode","enabled":"false"})"));
  assert(run(R"({"cmd":"set_display_config","rotation":1,"offset":80})"));
  assert(!run(R"({"cmd":"set_display_config","rotation":4,"offset":80})"));
  assert(!run(R"({"cmd":"set_display_config","rotation":0,"offset":96})"));
  assert(!run(R"({"cmd":"set_grow_mode","mode":"GROWTH"})"));
  assert(!run(R"({"cmd":"pump_test","action":"start"})"));
  assert(run(R"({"cmd":"set_rest_mode","enabled":false})"));
  assert(run(R"({"cmd":"test_output","output":"OUT12_1","state":true})"));
  assert(!run(R"({"cmd":"test_output","output":"OUT12_1","percent":50})"));
  assert(!run(R"({"cmd":"test_output","output":"0_10V_OUT_1","percent":"100"})"));
  assert(run(R"({"cmd":"test_output","output":"0_10V_OUT_1","percent":42.5})"));
  testing = true;
  assert(!run(R"({"cmd":"test_output","output":"OUT12_1","state":true})"));
  testing = false;
  setup = true;
  assert(!run(R"({"cmd":"set_rest_mode","enabled":false})"));
  setup = false;
  assert(!run(R"({"cmd":"test_output","output":"OUT12_1","state":true})", true));
  assert(!run(
      R"({"cmd":"test_output","output":"OUT12_1","state":true,"cmd_id":"expired","expires_at_s":1800000000})",
      true));
  clockReady = false;
  assert(!run(
      R"({"cmd":"test_output","output":"OUT12_1","state":true,"cmd_id":"clock","expires_at_s":1800000010})",
      true));
  clockReady = true;
  const char* packet =
      R"({"cmd":"test_output","output":"OUT12_1","state":true,"cmd_id":"valid","expires_at_s":1800000010})";
  assert(!run(packet, true));
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

  // Assignment is atomic, local-only, persisted and electrically validated.
  const char* assignment =
      R"({"cmd":"configure_equipment","equipment":[{"id":"dev1","profile":"pump","name":"Pumpe","output":"OUT5_1","voltage":5,"max_runtime_ms":2000},{"id":"dev2","profile":"light_0_10v","name":"Licht","output":"0_10V_OUT_1","power_output":"OUT12_1","power_voltage":12}]})";
  assert(run(assignment));
  assert(!run(
      R"({"cmd":"configure_equipment","equipment":[{"id":"dev1","profile":"pump","name":"Pumpe","output":"OUT5_1","voltage":12}]})"));
  assert(!run(
      R"({"cmd":"configure_equipment","equipment":[{"id":"dev1","profile":"pump","name":"Pumpe","output":"0_10V_OUT_1","voltage":12}]})"));
  assert(!run(
      R"({"cmd":"configure_equipment","equipment":[{"id":"dev1","profile":"pump","name":"Pumpe","output":"OUT5_1","voltage":5},{"id":"dev2","profile":"switch","name":"X","output":"OUT5_1","voltage":5}]})"));
  assert(!run(
      R"({"cmd":"configure_equipment","equipment":[],"cmd_id":"remote-config","expires_at_s":1800000010})",
      true));
  testNvs::failKey = "founder-devices/configuration";
  assert(!run(R"({"cmd":"configure_equipment","equipment":[]})"));
  testNvs::failKey = "";
  founderEquipment::begin();  // Simulate persisted configuration reload.
  assert(founderEquipment::settingsReady() && founderEquipment::hasAssignments());
  assert(founderApp::setRest(false));
  assert(!run(assignment));  // Remapping while active is forbidden.
  assert(!run(R"({"cmd":"test_output","output":"OUT5_1","state":true})"));
  assert(!run(R"({"cmd":"test_output","output":"OUT5_1","state":false})"));
  assert(!run(R"({"cmd":"set_device","id":"dev1","state":true})"));
  assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"locked"})"));
  nowMs = 600000;
  assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":2001,"event_id":"over-limit"})"));
  testNvs::failKey = "founder-devices/events";
  assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"write-fails"})"));
  assert(founderOutputs::applied(1) == 0);
  testNvs::failKey = "";
  assert(run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"pulse-1"})"));
  assert(founderOutputs::applied(1) == 100 && gpio[5] == HIGH);
  assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":1000,"event_id":"extend"})"));
  nowMs += 99;
  testTimer::fire();
  assert(founderOutputs::applied(1) == 100 && gpio[5] == HIGH);
  nowMs += 1;
  testTimer::fire();
  assert(founderOutputs::applied(1) == 0 && gpio[5] == LOW);  // No app loop involved.
  assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"pulse-1"})"));
  founderEquipment::begin();  // Already-consumed event also rejected after reload.
  assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"pulse-1"})"));
  assert(!run(
      R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"far","cmd_id":"far","expires_at_s":1800000061})",
      true));
  assert(run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"pulse-2"})"));
  assert(run(R"({"cmd":"stop_device","id":"dev1"})"));
  assert(founderOutputs::applied(1) == 0);
  const char* remotePulse =
      R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"remote-pulse","cmd_id":"remote-pulse","expires_at_s":1800000010})";
  assert(run(remotePulse, true));
  assert(!run(remotePulse, true));
  assert(run(R"({"cmd":"stop_device","id":"dev1"})"));
  assert(run(R"({"cmd":"set_device","id":"dev2","percent":42})"));
  assert(founderOutputs::applied(0) == 100 && founderOutputs::applied(3) == 42);
  assert(run(R"({"cmd":"stop_device","id":"dev2"})"));
  assert(founderOutputs::applied(0) == 0 && founderOutputs::applied(3) == 0);
  assert(run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"pulse-rest"})"));
  assert(founderApp::setRest(true));
  assert(founderOutputs::applied(1) == 0);
  assert(!deserializeJson(status, founderApp::status()));
  assert(status["equipment"].size() == 2 && status["device_catalog"].size() == 7);
  assert(status["outputs"][1]["assigned"].as<bool>());
  assert(status["equipment"][0]["max_runtime_ms"].as<unsigned>() == 2000);
  if (argc > 1) {
    std::cout << founderApp::status().c_str() << "\n";
  }
  // Stop-all cancels devices and fan requests without changing the release state.
  assert(founderApp::setRest(false));
  assert(run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"global-stop"})"));
  assert(run(R"({"cmd":"set_device","id":"dev2","percent":60})"));
  manualFans[0] = 50;
  manualFans[1] = 70;
  assert(run(R"({"cmd":"stop_all"})"));
  assert(!restMode_isEnabled());
  assert(founderOutputs::applied(0) == 0 && founderOutputs::applied(1) == 0 &&
         founderOutputs::applied(3) == 0);
  assert(manualFans[0] == 0 && manualFans[1] == 0);
  manualFans[0] = 80;
  manualFans[1] = 40;
  assert(founderApp::setRest(true));
  assert(founderApp::setRest(false));
  assert(manualFans[0] == 0 && manualFans[1] == 0);
  failFanSave = true;
  assert(!run(R"({"cmd":"stop_all"})"));
  assert(restMode_isEnabled());  // Failed durable fan stop falls back to lock.
  assert(!founderApp::setRest(false));
  assert(restMode_isEnabled());
  failFanSave = false;
  // Corrupt configuration never leaves old output ownership active.
  testNvs::data["founder-devices/configuration"] = {'x'};
  founderEquipment::begin();
  assert(!founderEquipment::settingsReady() && !founderEquipment::hasAssignments());
  assert(founderOutputs::applied(0) == 0 && founderOutputs::applied(1) == 0);

  founderOutputs::inhibit();
  assert(!run(R"({"cmd":"set_rest_mode","enabled":false})"));
  std::cout << "Founder production command parsing, deadlines, duplicate suppression, storage "
               "rollback and status JSON passed\n";
}

namespace founderDisplay {
bool present() {
  return true;
}
unsigned rotation() {
  return 0;
}
unsigned offset() {
  return 80;
}
bool configure(unsigned r, unsigned o) {
  return r <= 3 && o <= 95;
}
}  // namespace founderDisplay
