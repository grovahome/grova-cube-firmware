#include "controller/app.h"

#include <Preferences.h>

#include "boards/grova_core_s3.h"
#include "device_identity.h"
#include "firmware_info.h"
#include "controller/automation.h"
#include "controller/devices.h"
#include "controller/display.h"
#include "controller/equipment.h"
#include "controller/event_log.h"
#include "controller/history.h"
#include "controller/i2c_bus.h"
#include "controller/network.h"
#include "controller/outputs.h"
#include "controller/rtc.h"
#include "modules/command_deadline.h"
#include "modules/fan.h"
#include "modules/fan_control.h"
#include "modules/fan_policy.h"
#include "modules/rest_mode.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
#include "modules/wifi_ota.h"
static Preferences prefs;
static bool settingsReady = false;
struct Targets {
  uint32_t version = 1;
  float temperature = 23, humidity = 65;
};
static Targets targets;
static char identity[32];
float getTargetTemp() {
  return targets.temperature;
}
float getTargetHum() {
  return targets.humidity;
}
namespace founderApp {
static RecentCommandIds recent;
static bool resumePlans = false, resumeFailed = false, intentActive = false, bootReleased = false;
static constexpr uint32_t IntentRun = 0x47525631, IntentPause = 0x50415553;
void bootSafetyBegin() {
  founderOutputs::preinit();
  fan_preinit();
  restMode_begin();
  resumeFailed = false;
  Preferences bootState;
  bootReleased = bootState.begin("vgrow-rest", true) && !bootState.getBool("enabled", true);
  resumePlans = intentActive = false;
  restMode_setEnabled(true, false);
}
static bool allSettingsReady() {
  return settingsReady && founderEquipment::settingsReady() && restMode_settingsReady() &&
         time_settingsReady() && fanPolicy_settingsReady() && wifiOTA_storageReady();
}

static bool validTargets(const Targets& t) {
  return t.version == 1 && isfinite(t.temperature) && isfinite(t.humidity) &&
         t.temperature >= -20 && t.temperature <= 60 && t.humidity >= 0 && t.humidity <= 100;
}
void settingsBegin() {
  char suffix[13];
  grovaMacSuffix(ESP.getEfuseMac(), suffix);
  snprintf(identity, sizeof(identity), "grova-core-%s", suffix);
  settingsReady = prefs.begin("founder-app", false);
  intentActive = resumePlans = false;
  if (settingsReady && prefs.isKey("operation")) {
    uint32_t record[2] = {};
    if (prefs.getBytesLength("operation") != sizeof(record) ||
        prefs.getBytes("operation", record, sizeof(record)) != sizeof(record) ||
        (record[0] != IntentRun && record[0] != IntentPause) || record[1] != ~record[0]) {
      settingsReady = false;
    } else {
      intentActive = resumePlans = bootReleased && record[0] == IntentRun;
    }
  }
  if (settingsReady && prefs.getBytesLength("targets") == sizeof(targets)) {
    Targets stored;
    if (prefs.getBytes("targets", &stored, sizeof(stored)) == sizeof(stored) &&
        validTargets(stored)) {
      targets = stored;
    }
  }
}
const char* deviceId() {
  return identity;
}
bool setRest(bool enabled) {
  resumePlans = false;
  if (!enabled && (wifiOTA_requiresSetup() || wifiOTA_isTesting() || founderOutputs::inhibited() ||
                   !allSettingsReady() || !founderOutputs::ready())) {
    return false;
  }
  if (enabled) {
    intentActive = false;
    restMode_setEnabled(true, false);
    founderOutputs::interlock(true, wifiOTA_requiresSetup());
    founderEquipment::stopAll();
    fan_forceOff();
  }
  const uint32_t marker = enabled ? IntentPause : IntentRun;
  const uint32_t record[2] = {marker, ~marker};
  const bool savedIntent =
      settingsReady && prefs.putBytes("operation", record, sizeof(record)) == sizeof(record);
  // Desired operation and Rest must BOTH acknowledge release. A torn save stays paused.
  const bool first = fan_setManual(1, 0, true);
  const bool second = fan_setManual(2, 0, true);
  const bool ok = restMode_setEnabled(enabled || !savedIntent || !first || !second, true);
  founderOutputs::interlock(restMode_isEnabled(), wifiOTA_requiresSetup());
  if (enabled || !savedIntent || !ok || !first || !second) {
    founderEquipment::stopAll();
    fan_forceOff();
  }
  const bool success = savedIntent && ok && first && second;
  if (success) {
    intentActive = !enabled;
    resumeFailed = false;
  }
  return success;
}
void loop() {
  if (intentActive && restMode_isEnabled() && !resumeFailed) {
    resumePlans = true;
  }
  if (resumePlans && founderEquipment::hasPlans() && isTimeSynced() && allSettingsReady() &&
      founderOutputs::ready() && !founderOutputs::inhibited() && !wifiOTA_requiresSetup() &&
      !wifiOTA_isTesting()) {
    resumeFailed = !setRest(false);
  }
  founderEquipment::loop();
  founderEquipment::recordEvents();
  founderHistory::loop();
}
bool hasFault() {
  return resumeFailed || !allSettingsReady() || !founderOutputs::ready() || fan_hasTachoFault();
}
String status() {
  JsonDocument j;
  j["cube_id"] = identity;
  j["firmware_name"] = "GROVA";
  j["firmware_version"] = GROVA_FIRMWARE_VERSION;
  j["firmware_build_env"] = GROVA_BUILD_ENV;
  j["firmware_build_date"] = GROVA_BUILD_DATE;
  j["firmware_build_time"] = GROVA_BUILD_TIME;
  j["hardware_profile"] = GROVA_BOARD_PROFILE_ID;
  j["uptime_s"] = millis() / 1000;
  j["free_heap"] = ESP.getFreeHeap();
  j["flash_bytes"] = ESP.getFlashChipSize();
  j["psram_bytes"] = ESP.getPsramSize();
  bool settingsOk = allSettingsReady();
  j["warning"] = resumeFailed               ? "RESUME_FAILED"
                 : !settingsOk              ? "SETTINGS"
                 : !founderOutputs::ready() ? "OUTPUT_INIT"
                 : fan_hasTachoFault()      ? "FAN_FAULT"
                                            : "OK";
  j["settings_ok"] = settingsOk;
  j["outputs_ready"] = founderOutputs::ready();
  j["oled_present"] = founderDisplay::present();
  j["display"]["controller"] = founderDisplay::controller();
  j["display"]["rotation"] = founderDisplay::rotation();
  j["display"]["offset"] = founderDisplay::offset();
  j["healthy"] = !hasFault();
  j["wifi_connected"] = wifiOTA_isConnected();
  j["ota_ready"] = wifiOTA_otaReady();
  j["ip"] = wifiOTA_getIP();
  j["mqtt_connected"] = founderNetwork::connected();
  j["time_synced"] = isTimeSynced();
  j["time_source"] = time_getSourceName();
  j["epoch_s"] = getEpochSeconds();
  j["rest_mode"]["enabled"] = restMode_isEnabled();
  j["automation_supported"] = true;
  j["schedule_timezone"] = "Europe/Berlin";
  j["schedule_resume_pending"] = resumePlans;
  j["schedule_run_enabled"] = intentActive;
  j["schedule_resume_error"] = resumeFailed;
  j["output_roles_assigned"] = founderEquipment::hasAssignments();
  j["mux_present"] = founderBus::hasMux();
  j["rtc"]["type"] = "RV-3032";
  j["rtc"]["enabled"] = rtc_isEnabled();
  j["rtc"]["present"] = rtc_isPresent();
  j["rtc"]["valid"] = rtc_hasValidTime();
  j["rtc"]["backup_configured"] = founderRtc::backupConfigured();
  j["rtc"]["used_for_boot"] = rtc_wasUsedForBoot();
  j["rtc"]["last_write_ok"] = rtc_lastWriteOk();
  JsonArray signals = j["signals"].to<JsonArray>();
  for (int n = 0; n < SIGNAL_COUNT; ++n) {
    auto id = static_cast<SignalId>(n);
    auto v = signalRegistry_read(id);
    JsonObject item = signals.add<JsonObject>();
    item["id"] = signalRegistry_name(id);
    item["unit"] = signalRegistry_unit(id);
    item["valid"] = v.valid;
    item["source"] = v.source;
    if (v.valid) {
      item["value"] = v.value;
      item["age_ms"] = signalRegistry_ageMs(id);
    } else {
      item["value"] = nullptr;
      item["age_ms"] = nullptr;
    }
  }
  const char* keys[] = {"temperature_c", "humidity_pct", "pressure_hpa",
                        "co2_ppm",       "lux",          "uv_index"};
  for (int n = 0; n < SIGNAL_COUNT; ++n) {
    auto v = signalRegistry_read(static_cast<SignalId>(n));
    if (v.valid) {
      j["environment"][keys[n]] = v.value;
    } else {
      j["environment"][keys[n]] = nullptr;
    }
  }
  j["temp_c"] = j["environment"]["temperature_c"];
  j["hum_pct"] = j["environment"]["humidity_pct"];
  j["environment"]["temperature_source"] = signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C).source;
  j["environment"]["humidity_source"] = signalRegistry_read(SIGNAL_CLIMATE_HUMIDITY_PCT).source;
  j["target_temp_c"] = targets.temperature;
  j["target_hum_pct"] = targets.humidity;
  j["targets"]["temperature_c"] = targets.temperature;
  j["targets"]["humidity_pct"] = targets.humidity;
  for (int fan = 1; fan <= 2; ++fan) {
    String key = "fan";
    key += fan;
    auto f = j[key].to<JsonObject>();
    f["enabled"] = founderEquipment::fanEnabled(fan);
    f["mode"] = fan_getModeName(fan);
    f["current_pct"] = fan == 1 ? getFanPercent() : getFan2Percent();
    f["target_pct"] = fan == 1 ? getFanTargetPercent() : getFan2TargetPercent();
    f["rpm"] = fan == 1 ? getFanRPM() : getFan2RPM();
    f["tacho_fault"] = fan_getTachoFault(fan);
    f["reason"] = fan_getReasonName(fan);
  }
  founderNetwork::appendConfig(j["mqtt"].to<JsonObject>());
  String out;
  serializeJson(j, out);
  out.remove(out.length() - 1);
  out += ',';
  founderOutputs::appendJson(out);
  out += ',';
  founderDevices::appendJson(out);
  out += ',';
  founderEquipment::appendJson(out);
  out += ',';
  founderEvents::appendJson(out);
  out += ',';
  fanControl_appendJson(out);
  out += '}';
  return out;
}
static bool result(String& r, bool ok, const char* message) {
  JsonDocument j;
  j["ok"] = ok;
  j["message"] = message;
  serializeJson(j, r);
  return ok;
}
bool command(const String& body, String& response, bool remote) {
  response = "";
  JsonDocument j;
  if (body.length() > founderEquipment::BackupMaxBytes || deserializeJson(j, body) ||
      !j.is<JsonObject>() || !j["cmd"].is<const char*>()) {
    return result(response, false, "Invalid command JSON");
  }
  const char* cmd = j["cmd"];
  if (j["expires_at_s"].isNull() == false && !j["expires_at_s"].is<uint32_t>()) {
    return result(response, false, "Invalid deadline");
  }
  uint32_t expiry = j["expires_at_s"] | 0UL;
  const char* request = j["cmd_id"] | "";
  if (strlen(request) > 47 || (remote && (!*request || !expiry)) ||
      !commandDeadlineAllows(expiry, isTimeSynced(), getEpochSeconds())) {
    return result(response, false, "Missing or expired command deadline / identity");
  }
  if (!recent.accept(request)) {
    return result(response, false, "Duplicate command");
  }
  if (wifiOTA_requiresSetup() || wifiOTA_isTesting() || founderOutputs::inhibited()) {
    return result(response, false, "Setup or OTA interlock active");
  }
  bool ok = false;
  if (!strcmp(cmd, "set_rest_mode") && j["enabled"].is<bool>()) {
    ok = setRest(j["enabled"].as<bool>());
  } else if (!strcmp(cmd, "stop_all")) {
    bool paused = setRest(true);
    founderEquipment::stopAll();
    founderOutputs::stopAll();
    fan_forceOff();
    bool first = fan_setManual(1, 0, true);
    bool second = fan_setManual(2, 0, true);
    fan_forceOff();
    ok = paused && first && second;
    if (!ok) {
      setRest(true);
    }
  } else if (!strcmp(cmd, "restore_equipment") && !remote) {
    String message;
    if (!founderEquipment::restoreBackup(j.as<JsonObjectConst>(), message, true)) {
      return result(response, false, message.c_str());
    }
    if (!setRest(true)) {
      return result(response, false, "Could not persist Pause; configuration unchanged");
    }
    const bool restored = founderEquipment::restoreBackup(j.as<JsonObjectConst>(), message, false);
    return result(response, restored,
                  restored ? "Backup restored; operation remains paused" : message.c_str());
  } else if (!strcmp(cmd, "configure_equipment") && !remote) {
    String message;
    if (!j["start_plans"].isNull() && !j["start_plans"].is<bool>()) {
      return result(response, false, "Invalid plan start flag");
    }
    ok = founderEquipment::configure(j.as<JsonObjectConst>(), message);
    if (ok && (j["start_plans"] | false) && founderEquipment::hasPlans()) {
      ok = setRest(false);
      message = ok ? "Devices and automation saved; operation enabled"
                   : "Saved, but operation remains paused; check storage and interlocks";
    }
    return result(response, ok, message.c_str());
  } else if (!strcmp(cmd, "set_device") || !strcmp(cmd, "run_device") ||
             !strcmp(cmd, "stop_device")) {
    if (remote && !strcmp(cmd, "run_device") && expiry - getEpochSeconds() > 60) {
      return result(response, false, "Timed command deadline must be within 60 seconds");
    }
    if (!strcmp(cmd, "stop_device") && founderEquipment::planEnabled(j["id"] | "")) {
      bool paused = setRest(true);
      return result(response, paused, "Automatic operation paused; all outputs off");
    }
    founderOutputs::interlock(restMode_isEnabled(), wifiOTA_requiresSetup());
    String message;
    ok = founderEquipment::operate(j.as<JsonObjectConst>(), message);
    return result(response, ok, message.c_str());
  } else if (!strcmp(cmd, "test_output") && !remote && j["output"].is<const char*>()) {
    founderOutputs::interlock(restMode_isEnabled(), wifiOTA_requiresSetup());
    if (j["state"].is<bool>()) {
      ok = founderOutputs::set(j["output"], j["state"].as<bool>() ? 100 : 0);
    } else if (j["percent"].is<float>()) {
      ok = founderOutputs::set(j["output"], j["percent"].as<float>());
    }
  } else if (!strcmp(cmd, "set_fan_manual") && j["fan"].is<int>() && j["percent"].is<int>()) {
    int fan = j["fan"], percent = j["percent"];
    if (fan >= 1 && fan <= 2 && percent >= 0 && percent <= 100 &&
        (percent == 0 || (allSettingsReady() && founderEquipment::fanEnabled(fan)))) {
      if (founderEquipment::fanControlled(fan)) {
        return result(response, percent == 0 && setRest(true),
                      "Fan is scheduled or automatic; pause or edit configuration");
      }
      ok = founderEquipment::setFanManual(fan, percent);
      if (!ok) {
        setRest(true);
      }
    }
  } else if (!strcmp(cmd, "set_fan_auto") || !strcmp(cmd, "set_fan_policy")) {
    return result(
        response, false,
        "Use per-device automation in hardware settings; legacy fan policies are disabled");
  } else if (!strcmp(cmd, "set_targets") && j["temperature_c"].is<float>() &&
             j["humidity_pct"].is<float>()) {
    Targets next;
    next.temperature = j["temperature_c"];
    next.humidity = j["humidity_pct"];
    ok = validTargets(next) && settingsReady &&
         prefs.putBytes("targets", &next, sizeof(next)) == sizeof(next);
    if (ok) {
      targets = next;
    }
  } else if (!strcmp(cmd, "set_display_config") && j["rotation"].is<unsigned>() &&
             j["offset"].is<unsigned>()) {
    const JsonVariantConst profile = j["controller"];
    if (profile.isUnbound()) {
      ok = founderDisplay::configure(j["rotation"], j["offset"]);
    } else if (!remote && profile.is<const char*>()) {
      ok = founderDisplay::configure(j["rotation"], j["offset"], profile.as<const char*>());
    }
  } else if (!strcmp(cmd, "set_rtc_config") && j["enabled"].is<bool>()) {
    ok = rtc_setEnabled(j["enabled"]);
  } else if (!strcmp(cmd, "set_mqtt_config") && !remote) {
    ok = founderNetwork::configure(j.as<JsonObjectConst>());
  } else {
    return result(response, false, "Unsupported command or invalid parameters");
  }
  return result(response, ok, ok ? "Applied" : "Rejected: check Rest, range or storage");
}
}  // namespace founderApp
