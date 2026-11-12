#include "founder/app.h"

#include <Preferences.h>
#include <driver/timer.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

#include "founder/device_profiles.h"
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
static unsigned minuteOfDay = 8 * 60;
int getHour() {
  return minuteOfDay / 60;
}
int getMinute() {
  return minuteOfDay % 60;
}
int getDateKey() {
  return 2026001;
}
unsigned long millis() {
  return nowMs;
}
static int gpio[49] = {};
void digitalWrite(int pin, int value) {
  gpio[pin] = value;
}
void pinMode(int, int) {}
static bool failOutputInit = false;
double ledcSetup(int, double f, int) {
  return failOutputInit ? 0 : f;
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
void fan_preinit() {}
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

static JsonDocument statusJson() {
  JsonDocument j;
  assert(!deserializeJson(j, founderApp::status()));
  return j;
}
static void saveNvs(const char* path) {
  std::ofstream file(path, std::ios::binary);
  assert(file);
  uint32_t n = testNvs::data.size();
  file.write((char*)&n, sizeof(n));
  for (const auto& entry : testNvs::data) {
    uint32_t k = entry.first.size(), v = entry.second.size();
    file.write((char*)&k, sizeof(k));
    file.write(entry.first.data(), k);
    file.write((char*)&v, sizeof(v));
    file.write((const char*)entry.second.data(), v);
  }
  assert(file.good());
}
static void loadNvs(const char* path) {
  std::ifstream file(path, std::ios::binary);
  assert(file);
  uint32_t n;
  file.read((char*)&n, sizeof(n));
  assert(n < 100);
  while (n--) {
    uint32_t k, v;
    file.read((char*)&k, sizeof(k));
    assert(k < 200);
    std::string key(k, ' ');
    file.read(&key[0], k);
    file.read((char*)&v, sizeof(v));
    assert(v < 10000);
    std::vector<uint8_t> bytes(v);
    file.read((char*)bytes.data(), v);
    testNvs::data[key] = bytes;
  }
  assert(file.good());
}
static const char* rebootConfig =
    R"({"cmd":"configure_equipment","expected_revision":0,"equipment":[{"id":"dev1","name":"Pumpe neu","profile":"pump","output":"OUT5_1","voltage":5,"max_runtime_ms":1000},{"id":"dev2","name":"Licht neu","profile":"light_0_10v","output":"0_10V_OUT_1","power_output":"OUT12_1","power_voltage":12}],"fan_ports":[{"enabled":true,"name":"Zuluft"},{"enabled":false,"name":"Abluft"}]})";
int main(int argc, char** argv) {
  if (argc > 2 &&
      (!strcmp(argv[1], "--reboot-load") || !strcmp(argv[1], "--plan-load") ||
       !strcmp(argv[1], "--plan-paused-load") || !strcmp(argv[1], "--plan-no-intent-load") ||
       !strcmp(argv[1], "--plan-corrupt-intent-load") ||
       !strcmp(argv[1], "--plan-release-fail-load") || !strcmp(argv[1], "--plan-history-load") ||
       !strcmp(argv[1], "--plan-reserved-load"))) {
    loadNvs(argv[2]);
  }
  if (argc > 1 && !strcmp(argv[1], "--plan-no-intent-load")) {
    testNvs::data.erase("founder-app/operation");
  }
  if (argc > 1 && !strcmp(argv[1], "--plan-corrupt-intent-load")) {
    testNvs::data["founder-app/operation"] = {1, 2, 3};
  }
  if (argc > 1 && !strcmp(argv[1], "--storage-failure")) {
    testNvs::failBegin = "founder-devices";
  }
  if (argc > 1 && !strcmp(argv[1], "--empty-config")) {
    testNvs::data["founder-devices/configuration"] = {};
  }
  if (argc > 1 && !strcmp(argv[1], "--empty-ledger")) {
    testNvs::data["founder-devices/events"] = {};
  }
  if (argc > 1 && !strcmp(argv[1], "--driver-failure")) {
    failOutputInit = true;
  }
  if (argc > 1 && !strcmp(argv[1], "--app-storage-failure")) {
    testNvs::failBegin = "founder-app";
  }
  founderApp::bootSafetyBegin();
  founderApp::settingsBegin();
  founderOutputs::begin();
  founderEquipment::begin();
  signalRegistry_begin();
  if (argc > 1 && !strncmp(argv[1], "--plan-", 7)) {
    if (!strcmp(argv[1], "--plan-no-intent-load") ||
        !strcmp(argv[1], "--plan-corrupt-intent-load") ||
        !strcmp(argv[1], "--plan-release-fail-load")) {
      founderApp::loop();
      assert(restMode_isEnabled() && founderOutputs::applied(0) == 0 &&
             founderOutputs::applied(1) == 0);
      if (!strcmp(argv[1], "--plan-corrupt-intent-load")) {
        assert(!founderApp::setRest(false));
      }
      return 0;
    }
    const char* config =
        R"({"cmd":"configure_equipment","start_plans":true,"equipment":[{"id":"dev1","profile":"light_switch","name":"Licht","output":"OUT12_1","voltage":12,"plan":{"enabled":true,"slots":[{"start_minute":480,"end_minute":540,"percent":100},{"start_minute":600,"end_minute":660,"percent":100}]}},{"id":"dev2","profile":"pump","name":"Wasser","output":"OUT5_1","voltage":5,"max_runtime_ms":1000,"plan":{"enabled":true,"slots":[{"start_minute":480,"runtime_ms":500},{"start_minute":481,"runtime_ms":500},{"start_minute":482,"runtime_ms":500}]}}]})";
    if (!strcmp(argv[1], "--plan-reserved-load")) {
      auto status = statusJson();
      auto r = status["equipment"][1]["last_run"];
      assert(r["state"].as<std::string>() == "INTERRUPTED_UNKNOWN" &&
             r["command_elapsed_ms"].isNull());
      nowMs = 600000;
      founderApp::loop();
      assert(founderOutputs::applied(1) == 0);
      return 0;
    }
    if (!strcmp(argv[1], "--plan-reserved-save")) {
      minuteOfDay = 450;
      assert(run(config));
      nowMs = 600000;
      minuteOfDay = 480;
      testNvs::afterWrite = [&](const std::string& key) {
        if (key == "founder-devices/plan-ledger") {
          saveNvs(argv[2]);
          throw 99;
        }
      };
      try {
        founderApp::loop();
        assert(false);
      } catch (int code) {
        assert(code == 99 && founderOutputs::applied(1) == 0);
      }
      testNvs::afterWrite = {};
      return 0;
    }
    if (!strcmp(argv[1], "--plan-history-load")) {
      auto status = statusJson();
      auto r = status["equipment"][1]["last_run"];
      assert(r["state"].as<std::string>() == "COMPLETED" &&
             r["command_elapsed_ms"].as<unsigned>() == 500);
      assert(!r["water_delivery_verified"].as<bool>());
      founderApp::loop();
      assert(founderOutputs::applied(1) == 0);
      return 0;
    }
    if (!strcmp(argv[1], "--plan-journal-faults")) {
      minuteOfDay = 450;
      assert(run(config));
      nowMs = 600000;
      minuteOfDay = 480;
      testNvs::afterWrite = [](const std::string& key) {
        if (key == "founder-devices/plan-ledger") {
          testNvs::failKey = key;
        }
      };
      founderApp::loop();
      testNvs::afterWrite = {};
      assert(restMode_isEnabled() && founderOutputs::applied(1) == 0 &&
             !founderEquipment::settingsReady());
      auto state = statusJson();
      assert(state["equipment"][1]["last_run"]["state"].as<std::string>() == "RESERVED");
      assert(!state["equipment"][1]["last_run"]["durable"].as<bool>());
      testNvs::failKey = "";
      founderEquipment::begin();
      assert(statusJson()["equipment"][1]["last_run"]["state"].as<std::string>() ==
             "INTERRUPTED_UNKNOWN");
      assert(founderApp::setRest(false));
      minuteOfDay = 481;
      founderApp::loop();
      assert(founderOutputs::applied(1) == 100);
      nowMs += 500;
      testTimer::fire();
      testNvs::failKey = "founder-devices/plan-ledger";
      founderApp::loop();
      assert(restMode_isEnabled() && founderOutputs::applied(1) == 0);
      assert(statusJson()["equipment"][1]["last_run"]["state"].as<std::string>() == "STARTED");
      return 0;
    }
    if (!strcmp(argv[1], "--plan-history-stop")) {
      minuteOfDay = 450;
      assert(run(config));
      nowMs = 600000;
      minuteOfDay = 480;
      founderApp::loop();
      nowMs += 100;
      assert(founderApp::setRest(true));
      auto state = statusJson();
      auto r = state["equipment"][1]["last_run"];
      assert(r["state"].as<std::string>() == "INTERRUPTED" &&
             r["command_elapsed_ms"].as<unsigned>() == 100);
      assert(founderApp::setRest(false));
      founderApp::loop();
      assert(founderOutputs::applied(1) == 0);
      return 0;
    }
    if (!strcmp(argv[1], "--plan-journal-migration")) {
      minuteOfDay = 450;
      assert(run(config));
      const uint32_t old[] = {1, 0, 2917441920U, 0, 0, 0, 1861994284U};
      const auto* raw = reinterpret_cast<const uint8_t*>(old);
      testNvs::data["founder-devices/plan-ledger"] = std::vector<uint8_t>(raw, raw + sizeof(old));
      founderEquipment::begin();
      nowMs = 600000;
      minuteOfDay = 480;
      founderApp::loop();
      assert(founderOutputs::applied(1) == 0);
      assert(statusJson()["equipment"][1]["last_run"]["state"].as<std::string>() ==
             "LEGACY_UNKNOWN");
      return 0;
    }
    if (!strcmp(argv[1], "--plan-history-save")) {
      JsonDocument j;
      assert(!deserializeJson(j, config));
      j["equipment"][1]["plan"]["catch_up_minutes"] = 30;
      String request;
      serializeJson(j, request);
      minuteOfDay = 450;
      assert(run(request.c_str()));
      nowMs = 600000;
      founderApp::loop();
      assert(statusJson()["equipment"][1]["last_run"]["state"].as<std::string>() == "NONE");
      minuteOfDay = 491;
      founderApp::loop();
      auto started = statusJson();
      assert(started["equipment"][1]["last_run"]["state"].as<std::string>() == "STARTED");
      assert(started["equipment"][1]["last_run"]["scheduled_minute"].as<unsigned>() == 482);
      assert(founderOutputs::applied(1) == 100);
      const auto before = testNvs::data["founder-devices/plan-ledger"];
      founderApp::loop();
      assert(testNvs::data["founder-devices/plan-ledger"] == before);
      nowMs += 500;
      testTimer::fire();
      founderApp::loop();
      assert(statusJson()["equipment"][1]["last_run"]["state"].as<std::string>() == "COMPLETED");
      assert(founderOutputs::applied(1) == 0);
      founderApp::loop();
      assert(founderOutputs::applied(1) == 0);
      saveNvs(argv[2]);
      return 0;
    }
    if (!strcmp(argv[1], "--plan-load") || !strcmp(argv[1], "--plan-paused-load")) {
      assert(restMode_isEnabled() && founderOutputs::applied(0) == 0 &&
             founderOutputs::applied(1) == 0);
      clockReady = false;
      founderApp::loop();
      assert(restMode_isEnabled());
      clockReady = true;
      founderApp::loop();
      if (!strcmp(argv[1], "--plan-paused-load")) {
        assert(restMode_isEnabled() && founderOutputs::applied(0) == 0);
        return 0;
      }
      assert(statusJson()["equipment"][1]["last_run"]["state"].as<std::string>() ==
             "INTERRUPTED_UNKNOWN");
      assert(statusJson()["equipment"][1]["last_run"]["command_elapsed_ms"].isNull());
      assert(!restMode_isEnabled() && founderOutputs::applied(0) == 100);
      nowMs = 600000;
      founderApp::loop();
      assert(founderOutputs::applied(1) == 0);  // Reserved pulse must not replay.
      minuteOfDay = 481;
      founderApp::loop();
      assert(founderOutputs::applied(1) == 100);
      nowMs += 500;
      testTimer::fire();
      assert(founderOutputs::applied(1) == 0);
      minuteOfDay = 482;
      testNvs::failKey = "founder-devices/plan-ledger";
      founderApp::loop();
      assert(restMode_isEnabled() && founderOutputs::applied(0) == 0 &&
             founderOutputs::applied(1) == 0);
      assert(!founderEquipment::settingsReady());
      assert(founderApp::hasFault());
      return 0;
    }
    if (!strcmp(argv[1], "--plan-release-fail-save")) {
      testNvs::failKey = "vgrow-rest/enabled";
      assert(!run(config));
      founderApp::loop();
      assert(restMode_isEnabled() && founderOutputs::applied(0) == 0);
      saveNvs(argv[2]);
      return 0;
    }
    assert(run(config));
    assert(!restMode_isEnabled());
    founderApp::loop();
    assert(founderOutputs::applied(0) == 100 && founderOutputs::applied(1) == 0);
    if (!strcmp(argv[1], "--plan-integration")) {
      minuteOfDay = 540;
      founderApp::loop();
      assert(founderOutputs::applied(0) == 0);
      minuteOfDay = 600;
      founderApp::loop();
      assert(founderOutputs::applied(0) == 100);
      clockReady = false;
      founderApp::loop();
      assert(founderOutputs::applied(0) == 0);
      clockReady = true;
      founderApp::loop();
      assert(founderOutputs::applied(0) == 100);
      testing = true;
      restMode_setEnabled(true, false);
      founderApp::loop();
      assert(founderOutputs::applied(0) == 0);
      testing = false;
      founderApp::loop();
      assert(!restMode_isEnabled() && founderOutputs::applied(0) == 100);
      assert(!run(R"({"cmd":"set_device","id":"dev1","state":true})"));
      assert(run(R"({"cmd":"stop_all"})"));
      founderApp::loop();
      assert(restMode_isEnabled() && founderOutputs::applied(0) == 0);
      JsonDocument bad;
      assert(!deserializeJson(bad, config));
      bad["equipment"][0]["plan"]["slots"][1]["start_minute"] = 500;
      String body;
      serializeJson(bad, body);
      assert(!run(body.c_str()));
      assert(statusJson()["equipment"][0]["plan"]["slots"].size() == 2);
      bad["equipment"][0]["plan"]["slots"][1]["start_minute"] = 600;
      bad["equipment"][1]["plan"]["slots"][0]["runtime_ms"] = 1001;
      body = "";
      serializeJson(bad, body);
      assert(!run(body.c_str()));
      return 0;
    }
    if (!strcmp(argv[1], "--plan-corrupt")) {
      assert(founderApp::setRest(true));
      testNvs::data["founder-devices/plan-ledger"] = {};
      founderEquipment::begin();
      assert(!founderEquipment::settingsReady() && !founderApp::setRest(false));
      return 0;
    }
    nowMs = 600000;
    founderApp::loop();
    assert(founderOutputs::applied(1) == 100);
    if (!strcmp(argv[1], "--plan-paused-save")) {
      assert(run(R"({"cmd":"stop_all"})"));
    }
    saveNvs(argv[2]);
    return 0;
  }
  if (argc > 1 &&
      (!strcmp(argv[1], "--driver-failure") || !strcmp(argv[1], "--app-storage-failure"))) {
    assert(restMode_isEnabled());
    assert(!founderApp::setRest(false));
    for (unsigned i = 0; i < 5; ++i) {
      assert(founderOutputs::applied(i) == 0);
    }
    return 0;
  }

  if (argc > 1 && (!strcmp(argv[1], "--storage-failure") || !strcmp(argv[1], "--empty-config") ||
                   !strcmp(argv[1], "--empty-ledger"))) {
    assert(restMode_isEnabled() && !founderEquipment::settingsReady());
    assert(!founderApp::setRest(false));
    if (!strcmp(argv[1], "--storage-failure")) {
      assert(!run(rebootConfig));
    } else {
      assert(run(rebootConfig));
      if (!strcmp(argv[1], "--empty-config")) {
        assert(founderEquipment::settingsReady());
      } else {
        assert(!founderApp::setRest(false));
      }
    }
    return 0;
  }
  if (argc > 1 && (!strcmp(argv[1], "--blank") || !strcmp(argv[1], "--reboot-save"))) {
    auto j = statusJson();
    assert(j["equipment"].size() == 0 && !founderEquipment::hasAssignments());
    assert(restMode_isEnabled() && founderEquipment::settingsReady());
    for (JsonObject o : j["outputs"].as<JsonArray>()) {
      assert(!o["assigned"].as<bool>() && o["percent"].as<float>() == 0);
    }
    for (JsonObject f : j["fan_ports"].as<JsonArray>()) {
      assert(!f["enabled"].as<bool>());
    }
    if (!strcmp(argv[1], "--blank")) {
      std::cout << founderApp::status().c_str() << "\n";
      return 0;
    }
    assert(run(rebootConfig));
    assert(!run(rebootConfig));  // Stale revision cannot replace saved config.
    JsonDocument rename;
    assert(!deserializeJson(rename, rebootConfig));
    rename["expected_revision"] = 1;
    rename["equipment"][0]["name"] = "Kräuterpumpe";
    String renamed;
    serializeJson(rename, renamed);
    assert(run(renamed.c_str()));

    assert(founderApp::setRest(false));
    nowMs = 600000;
    assert(run(R"({"cmd":"run_device","id":"dev1","runtime_ms":1000,"event_id":"pre-crash"})"));
    assert(run(R"({"cmd":"set_device","id":"dev2","percent":60})"));
    assert(founderOutputs::applied(1) == 100 && founderOutputs::applied(3) == 60);
    saveNvs(argv[2]);
    return 0;  // Deliberately no graceful stop before the next fresh process.
  }
  if (argc > 1 && !strcmp(argv[1], "--reboot-load")) {
    auto j = statusJson();
    assert(restMode_isEnabled() && j["equipment"].size() == 2);
    assert(j["equipment"][0]["name"].as<std::string>() == "Kräuterpumpe");
    assert(j["equipment_revision"].as<unsigned>() == 2 && founderEquipment::fanEnabled(1));
    for (unsigned c = 0; c < 5; ++c) {
      assert(founderOutputs::applied(c) == 0);
    }
    assert(gpio[5] == LOW);
    assert(founderApp::setRest(false));
    nowMs = 600000;
    assert(!run(R"({"cmd":"run_device","id":"dev1","runtime_ms":1000,"event_id":"pre-crash"})"));
    assert(run(R"({"cmd":"configure_equipment","equipment":[]})") ==
           false);  // Must lock before removal.
    assert(founderApp::setRest(true));
    assert(run(R"({"cmd":"configure_equipment","expected_revision":2,"equipment":[]})"));
    assert(!run(R"({"cmd":"set_device","id":"dev1","state":true})"));
    assert(statusJson()["equipment"].size() == 0);
    return 0;
  }
  if (argc > 1 && !strcmp(argv[1], "--profile-matrix")) {
    unsigned count;
    const auto* profiles = founderEquipment::profiles(count);
    nowMs = 600000;
    for (unsigned i = 0; i < count; ++i) {
      const auto& p = profiles[i];
      assert(founderApp::setRest(true));
      JsonDocument req;
      req["cmd"] = "configure_equipment";
      auto row = req["equipment"].to<JsonArray>().add<JsonObject>();
      row["id"] = "dev1";
      row["name"] = p.id;
      row["profile"] = p.id;
      row["output"] = p.analog ? "0_10V_OUT_1" : "OUT5_1";
      if (!p.analog) {
        row["voltage"] = 5;
      }
      String body;
      serializeJson(req, body);
      assert(run(body.c_str()));
      assert(founderApp::setRest(false));
      JsonDocument action;
      action["id"] = "dev1";
      if (p.timed) {
        action["cmd"] = "run_device";
        action["runtime_ms"] = 100;
        action["event_id"] = p.id;
      } else {
        action["cmd"] = "set_device";
        if (p.analog) {
          action["percent"] = 25;
        } else {
          action["state"] = true;
        }
      }
      body = "";
      serializeJson(action, body);
      assert(run(body.c_str()));
      unsigned channel = p.analog ? 3 : 1;
      assert(founderOutputs::applied(channel) == (p.analog ? 25 : 100));
      assert(!run(p.analog ? R"({"cmd":"test_output","output":"0_10V_OUT_1","percent":50})"
                           : R"({"cmd":"test_output","output":"OUT5_1","state":true})"));
      if (p.timed) {
        nowMs += 100;
        testTimer::fire();
        assert(founderOutputs::applied(channel) == 0);
      }
      assert(run(R"({"cmd":"stop_device","id":"dev1"})"));
      assert(founderOutputs::applied(channel) == 0);
    }
    std::cout << "All 7 native profile commands and stops passed\n";
    return 0;
  }
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
  assert(status["automation_supported"].as<bool>());

  // Assignment is atomic, local-only, persisted and electrically validated.
  const char* assignment =
      R"({"cmd":"configure_equipment","equipment":[{"id":"dev1","profile":"pump","name":"Pumpe","output":"OUT5_1","voltage":5,"max_runtime_ms":2000},{"id":"dev2","profile":"light_0_10v","name":"Licht","output":"0_10V_OUT_1","power_output":"OUT12_1","power_voltage":12}]})";
  assert(run(assignment));
  assert(!run(R"({"cmd":"configure_equipment","expected_revision":0,"equipment":[]})"));
  assert(!run(R"({"cmd":"configure_equipment","expected_revision":"1","equipment":[]})"));
  assert(!run(
      R"({"cmd":"configure_equipment","equipment":[{"id":"dev1","profile":"pump","name":"   ","output":"OUT5_1","voltage":5}]})"));
  assert(statusJson()["equipment_revision"].as<unsigned>() == 1);

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
  // Stop-all durably pauses automatic operation so the next loop cannot restart it.
  assert(founderApp::setRest(false));
  assert(run(R"({"cmd":"run_device","id":"dev1","runtime_ms":100,"event_id":"global-stop"})"));
  assert(run(R"({"cmd":"set_device","id":"dev2","percent":60})"));
  manualFans[0] = 50;
  manualFans[1] = 70;
  assert(run(R"({"cmd":"stop_all"})"));
  assert(restMode_isEnabled());
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
  assert(!run(R"({"cmd":"set_fan_manual","fan":1,"percent":50})"));
  assert(!run(R"({"cmd":"set_fan_auto","fan":1})"));
  assert(!run(R"({"cmd":"configure_equipment","equipment":[],"fan_ports":[true,false]})"));
  assert(run(
      R"({"cmd":"configure_equipment","equipment":[],"fan_ports":[{"enabled":true,"name":"Zuluft"},{"enabled":false,"name":"Abluft"}]})"));
  assert(founderEquipment::fanEnabled(1) && !founderEquipment::fanEnabled(2));
  assert(founderEquipment::hasAssignments());
  founderEquipment::begin();
  assert(founderEquipment::fanEnabled(1));
  testNvs::failKey = "founder-devices/configuration";
  assert(!run(
      R"({"cmd":"configure_equipment","equipment":[],"fan_ports":[{"enabled":false,"name":"Zuluft"},{"enabled":false,"name":"Abluft"}]})"));
  assert(founderEquipment::fanEnabled(1));
  testNvs::failKey = "";
  assert(founderApp::setRest(false));
  assert(run(R"({"cmd":"set_fan_manual","fan":1,"percent":50})"));
  assert(!run(R"({"cmd":"set_fan_manual","fan":2,"percent":50})"));
  assert(founderApp::setRest(true));
  assert(run(
      R"({"cmd":"configure_equipment","equipment":[],"fan_ports":[{"enabled":false,"name":"Zuluft"},{"enabled":false,"name":"Abluft"}]})"));
  assert(!founderEquipment::hasAssignments());
  founderEquipment::begin();
  assert(!founderEquipment::fanEnabled(1));
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
const char* controller() {
  return "SSD1317";
}
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
