#include "founder/equipment.h"

#include <Preferences.h>
#include <stddef.h>

#include <memory>
#include <new>

#include "founder/device_profiles.h"
#include "founder/outputs.h"
#include "founder/schedule_policy.h"
#include "modules/rest_mode.h"
#include "modules/time_sync.h"
#include "modules/wifi_ota.h"
namespace founderEquipment {
struct Device {
  const Profile* profile = nullptr;
  char name[40] = {};
  int output = -1, power = -1;
  unsigned voltage = 0, maxRuntime = 0;
  founderSchedule::Plan plan;
};
static Device devices[Count];
static bool cleanText(const char*, size_t);
struct FanPort {
  bool enabled = false;
  char name[40] = {};
};
static FanPort fanPorts[2];
static bool parseFans(JsonObjectConst root, FanPort (&next)[2], String& error) {
  if (root["fan_ports"].isNull()) {
    return true;
  }
  if (!root["fan_ports"].is<JsonArrayConst>() || root["fan_ports"].size() != 2) {
    error = "Expected two fixed fan ports";
    return false;
  }
  for (unsigned i = 0; i < 2; ++i) {
    auto row = root["fan_ports"][i];
    if (!row["enabled"].is<bool>() || !row["name"].is<const char*>() ||
        !cleanText(row["name"], 39)) {
      error = "Invalid fan port configuration";
      return false;
    }
    next[i].enabled = row["enabled"];
    strcpy(next[i].name, row["name"]);
  }
  return true;
}

static Preferences settings;
static bool storage = false, configOk = false, eventsOk = false;
static uint32_t revision = 0;
static float requested[Count] = {};
struct Action {
  char id[48];
  uint8_t slot;
};
struct Ledger {
  uint32_t schema;
  uint32_t next;
  Action actions[16];
  uint32_t checksum;
};
static Ledger ledger;
struct PlanLedger {
  uint32_t schema = 1, token[Count] = {}, checksum = 0;
};
static PlanLedger planLedger;
static bool plansOk = true;
static uint32_t planChecksum(const PlanLedger& l) {
  uint32_t h = 2166136261UL;
  const auto* b = reinterpret_cast<const uint8_t*>(&l);
  for (size_t n = 0; n < offsetof(PlanLedger, checksum); ++n) {
    h = (h ^ b[n]) * 16777619UL;
  }
  return h;
}
static uint32_t checksum(const Ledger& l) {
  const auto* bytes = reinterpret_cast<const uint8_t*>(&l);
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < offsetof(Ledger, checksum); ++i) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}
static bool cleanText(const char* s, size_t limit) {
  if (!s || !*s || strlen(s) > limit) {
    return false;
  }
  bool nonblank = false;
  for (const char* p = s; *p; ++p) {
    if (*p != ' ') {
      nonblank = true;
    }
    if (uint8_t(*p) < 32) {
      return false;
    }
  }
  return nonblank;
}
static int slot(const char* id) {
  return id && strlen(id) == 4 && !strncmp(id, "dev", 3) && id[3] >= '1' && id[3] <= '5'
             ? id[3] - '1'
             : -1;
}
static bool parsePlan(JsonVariantConst value, Device& device, String& error) {
  if (value.isNull()) {
    return true;  // Existing configurations remain manual.
  }
  if (!value.is<JsonObjectConst>() || !value["enabled"].is<bool>() ||
      !value["slots"].is<JsonArrayConst>() || value["slots"].size() > founderSchedule::MaxSlots) {
    error = "Invalid daily plan";
    return false;
  }
  auto& plan = device.plan;
  plan.enabled = value["enabled"];
  for (JsonObjectConst item : value["slots"].as<JsonArrayConst>()) {
    if (!item["start_minute"].is<unsigned>() || item["start_minute"].as<unsigned>() >= 1440) {
      error = "Invalid start time";
      return false;
    }
    auto& slot = plan.slots[plan.count++];
    slot.start = item["start_minute"];
    if (device.profile->timed) {
      if (!item["runtime_ms"].is<unsigned>() ||
          item["runtime_ms"].as<unsigned>() > device.maxRuntime || !item["end_minute"].isNull() ||
          !item["percent"].isNull()) {
        error = "Invalid pulse plan";
        return false;
      }
      slot.runtimeMs = item["runtime_ms"];
    } else {
      if (!item["end_minute"].is<unsigned>() || item["end_minute"].as<unsigned>() > 1440 ||
          !item["percent"].is<unsigned>() || item["percent"].as<unsigned>() > 100 ||
          (!device.profile->analog && item["percent"].as<unsigned>() != 100) ||
          !item["runtime_ms"].isNull()) {
        error = "Invalid on/off window";
        return false;
      }
      slot.end = item["end_minute"];
      slot.percent = item["percent"];
    }
  }
  if (!founderSchedule::valid(plan, device.profile->timed, device.maxRuntime)) {
    error = "Daily plan is empty, overlapping or exceeds runtime limits";
    return false;
  }
  return true;
}
static void appendPlan(JsonObject row, const Device& device) {
  auto p = row["plan"].to<JsonObject>();
  p["enabled"] = device.plan.enabled;
  auto slots = p["slots"].to<JsonArray>();
  for (unsigned n = 0; n < device.plan.count; ++n) {
    const auto& s = device.plan.slots[n];
    auto item = slots.add<JsonObject>();
    item["start_minute"] = s.start;
    if (device.profile->timed) {
      item["runtime_ms"] = s.runtimeMs;
    } else {
      item["end_minute"] = s.end;
      item["percent"] = s.percent;
    }
  }
}
static bool parse(JsonObjectConst root, Device (&next)[Count], String& error) {
  if (!root["equipment"].is<JsonArrayConst>() || root["equipment"].size() > Count) {
    error = "Expected equipment array, max 5 devices";
    return false;
  }
  bool used[5] = {};
  bool slots[Count] = {};
  for (JsonObjectConst row : root["equipment"].as<JsonArrayConst>()) {
    if (!row["id"].is<const char*>() || !row["profile"].is<const char*>() ||
        !row["name"].is<const char*>() || !row["output"].is<const char*>()) {
      error = "Missing device identity, profile, name or output";
      return false;
    }
    int i = slot(row["id"]);
    const Profile* p = profile(row["profile"]);
    int out = channelIndex(row["output"]);
    if (i < 0 || slots[i] || !p || out < 0 || used[out] || !cleanText(row["name"], 39)) {
      error = "Unknown profile/channel, duplicate assignment or invalid name";
      return false;
    }
    if (channels()[out].analog != p->analog) {
      error = "Device requires a different output capability";
      return false;
    }
    Device& device = next[i];
    device.profile = p;
    device.output = out;
    strcpy(device.name, row["name"]);
    if (!p->analog) {
      if (!row["voltage"].is<unsigned>() ||
          row["voltage"].as<unsigned>() != channels()[out].voltage) {
        error = "Device voltage must match the selected 5V/12V supply";
        return false;
      }
      device.voltage = row["voltage"];
    } else if (!row["voltage"].isNull()) {
      error = "0-10V is a control signal; it does not supply the device";
      return false;
    }
    if (p->timed) {
      unsigned maximum = p->maxRuntimeMs;
      if (!row["max_runtime_ms"].isNull()) {
        if (!row["max_runtime_ms"].is<unsigned>()) {
          error = "Invalid maximum runtime";
          return false;
        }
        maximum = row["max_runtime_ms"];
      }
      if (!maximum || maximum > p->maxRuntimeMs) {
        error = "Maximum runtime exceeds the device protection limit";
        return false;
      }
      device.maxRuntime = maximum;
    } else if (!row["max_runtime_ms"].isNull()) {
      error = "Runtime applies only to timed devices";
      return false;
    }
    if (!parsePlan(row["plan"], device, error)) {
      return false;
    }
    used[out] = true;
    slots[i] = true;
    if (!row["power_output"].isNull()) {
      if (!p->optionalPower || !row["power_output"].is<const char*>() ||
          !row["power_voltage"].is<unsigned>()) {
        error = "This profile does not support a switched power companion";
        return false;
      }
      int power = channelIndex(row["power_output"]);
      if (power < 0 || used[power] || channels()[power].analog ||
          row["power_voltage"].as<unsigned>() != channels()[power].voltage) {
        error = "Invalid or occupied power output / wrong supply voltage";
        return false;
      }
      device.power = power;
      device.voltage = row["power_voltage"];
      used[power] = true;
    } else if (!row["power_voltage"].isNull()) {
      error = "Power voltage needs a power output";
      return false;
    }
  }
  return true;
}
static void apply(const Device (&next)[Count]) {
  int8_t owners[5] = {-1, -1, -1, -1, -1};
  bool pulse[5] = {};
  for (unsigned i = 0; i < Count; ++i) {
    if (next[i].profile) {
      owners[next[i].output] = i;
      pulse[next[i].output] = next[i].profile->timed;
      if (next[i].power >= 0) {
        owners[next[i].power] = i;
      }
    }
  }
  founderOutputs::assignOwners(owners, pulse);
  for (unsigned i = 0; i < Count; ++i) {
    devices[i] = next[i];
    requested[i] = 0;
  }
}
void begin() {
  planLedger = PlanLedger{};
  plansOk = true;
  Device empty[Count];
  apply(empty);
  revision = 0;
  fanPorts[0] = FanPort{};
  fanPorts[1] = FanPort{};
  strcpy(fanPorts[0].name, "Lüfter 1");
  strcpy(fanPorts[1].name, "Lüfter 2");
  FanPort initialFans[2] = {fanPorts[0], fanPorts[1]};
  storage = settings.begin("founder-devices", false);
  configOk = storage;
  eventsOk = storage;
  memset(&ledger, 0, sizeof(ledger));
  ledger.schema = 1;
  Device initial[Count];
  if (storage && settings.isKey("configuration")) {
    size_t size = settings.getBytesLength("configuration");
    std::unique_ptr<char[]> bytes(new (std::nothrow) char[4097]);
    JsonDocument root;
    String error;
    configOk = bool(bytes) && size > 0 && size <= 4096 &&
               settings.getBytes("configuration", bytes.get(), size) == size;
    if (configOk) {
      bytes[size] = 0;
      configOk = !deserializeJson(root, bytes.get()) && root["schema"].is<unsigned>() &&
                 root["schema"].as<unsigned>() == 1 && root["revision"].is<uint32_t>() &&
                 parse(root.as<JsonObjectConst>(), initial, error) &&
                 parseFans(root.as<JsonObjectConst>(), initialFans, error);
    }
    if (configOk) {
      revision = root["revision"];
    } else {
      Device empty[Count];
      apply(empty);
    }
  }
  if (configOk) {
    apply(initial);
    fanPorts[0] = initialFans[0];
    fanPorts[1] = initialFans[1];
  }
  if (storage && settings.isKey("plan-ledger")) {
    PlanLedger saved;
    plansOk = settings.getBytesLength("plan-ledger") == sizeof(saved) &&
              settings.getBytes("plan-ledger", &saved, sizeof(saved)) == sizeof(saved) &&
              saved.schema == 1 && saved.checksum == planChecksum(saved);
    if (plansOk) {
      planLedger = saved;
    }
  }
  if (storage && settings.isKey("events")) {
    Ledger saved;
    eventsOk = settings.getBytesLength("events") == sizeof(saved) &&
               settings.getBytes("events", &saved, sizeof(saved)) == sizeof(saved) &&
               saved.schema == 1 && saved.next < 16 && saved.checksum == checksum(saved);
    if (eventsOk) {
      for (const auto& action : saved.actions) {
        if (action.id[47] != 0 || action.slot >= Count) {
          eventsOk = false;
        }
      }
    }
    if (eventsOk) {
      ledger = saved;
    }
  }
}
bool fanEnabled(unsigned fan) {
  return fan >= 1 && fan <= 2 && fanPorts[fan - 1].enabled && settingsReady();
}
bool hasAssignments() {
  if (fanPorts[0].enabled || fanPorts[1].enabled) {
    return true;
  }
  for (const auto& d : devices) {
    if (d.profile) {
      return true;
    }
  }
  return false;
}
bool settingsReady() {
  return storage && configOk && eventsOk && plansOk;
}
bool configure(JsonObjectConst request, String& message) {
  if (!storage || !restMode_isEnabled()) {
    message = "Device assignment requires Rest and available storage";
    return false;
  }
  if (!request["expected_revision"].isNull() &&
      (!request["expected_revision"].is<uint32_t>() ||
       request["expected_revision"].as<uint32_t>() != revision)) {
    message = "Configuration changed; reload assignments before saving";
    return false;
  }
  if (revision == UINT32_MAX) {
    message = "Configuration revision exhausted";
    return false;
  }
  Device next[Count];
  FanPort nextFans[2] = {fanPorts[0], fanPorts[1]};
  if (!parseFans(request, nextFans, message) || !parse(request, next, message)) {
    return false;
  }
  JsonDocument root;
  root["schema"] = 1;
  root["revision"] = revision + 1;
  auto fans = root["fan_ports"].to<JsonArray>();
  for (const auto& fan : nextFans) {
    auto row = fans.add<JsonObject>();
    row["enabled"] = fan.enabled;
    row["name"] = fan.name;
  }
  JsonArray list = root["equipment"].to<JsonArray>();
  for (unsigned i = 0; i < Count; ++i) {
    if (next[i].profile) {
      auto row = list.add<JsonObject>();
      char id[5];
      snprintf(id, sizeof(id), "dev%u", i + 1);
      row["id"] = id;
      row["profile"] = next[i].profile->id;
      row["name"] = next[i].name;
      row["output"] = channels()[next[i].output].id;
      appendPlan(row, next[i]);
      if (!next[i].profile->analog) {
        row["voltage"] = next[i].voltage;
      }
      if (next[i].profile->timed) {
        row["max_runtime_ms"] = next[i].maxRuntime;
      }
      if (next[i].power >= 0) {
        row["power_output"] = channels()[next[i].power].id;
        row["power_voltage"] = next[i].voltage;
      }
    }
  }
  String bytes;
  serializeJson(root, bytes);
  if (bytes.length() > 4096 ||
      settings.putBytes("configuration", bytes.c_str(), bytes.length()) != bytes.length()) {
    message = "Device assignment was not saved; previous assignment remains";
    return false;
  }
  apply(next);
  fanPorts[0] = nextFans[0];
  fanPorts[1] = nextFans[1];
  ++revision;
  configOk = true;
  message = "Device assignment saved; all outputs off";
  return true;
}
static bool reserve(unsigned i, const char* event) {
  if (!eventsOk || !cleanText(event, 47)) {
    return false;
  }
  for (const auto& a : ledger.actions) {
    if (a.slot == i && !strcmp(a.id, event)) {
      return false;
    }
  }
  Ledger next = ledger;
  auto& action = next.actions[next.next];
  memset(&action, 0, sizeof(action));
  strcpy(action.id, event);
  action.slot = i;
  next.next = (next.next + 1) % 16;
  next.checksum = checksum(next);
  if (settings.putBytes("events", &next, sizeof(next)) != sizeof(next)) {
    return false;
  }
  ledger = next;
  return true;
}
static bool drive(unsigned i, float value) {
  const auto& d = devices[i];
  if (d.power >= 0 && value == 0) {
    bool a = founderOutputs::setDevice(i, d.output, 0);
    bool b = founderOutputs::setDevice(i, d.power, 0);
    return a && b;
  }
  if (d.power >= 0 && !founderOutputs::setDevice(i, d.power, 100)) {
    return false;
  }
  if (!founderOutputs::setDevice(i, d.output, value)) {
    if (d.power >= 0) {
      founderOutputs::setDevice(i, d.power, 0);
    }
    return false;
  }
  return true;
}
bool operate(JsonObjectConst request, String& message) {
  if (!request["id"].is<const char*>()) {
    message = "Missing device id";
    return false;
  }
  int i = slot(request["id"]);
  if (i < 0 || !devices[i].profile) {
    message = "Device is not configured";
    return false;
  }
  const auto& d = devices[i];
  const char* cmd = request["cmd"] | "";
  if (d.plan.enabled) {
    message = "Device is scheduled; pause or edit its plan first";
    return false;
  }
  if (!strcmp(cmd, "stop_device")) {
    requested[i] = 0;
    bool ok = drive(i, 0);
    message = ok ? "Device stopped" : "Stop failed";
    return ok;
  }
  if (!settingsReady() || restMode_isEnabled() || wifiOTA_requiresSetup() || wifiOTA_isTesting() ||
      founderOutputs::inhibited()) {
    message = "Device blocked by Rest, provisioning, OTA or storage";
    return false;
  }
  if (!strcmp(cmd, "run_device")) {
    if (!d.profile->timed || !request["runtime_ms"].is<unsigned>() ||
        !request["event_id"].is<const char*>()) {
      message = "Timed device requires runtime_ms and a unique event_id";
      return false;
    }
    unsigned runtime = request["runtime_ms"];
    if (!runtime || runtime > d.maxRuntime || millis() < d.profile->startupLockMs ||
        founderOutputs::applied(d.output) > 0 || !founderOutputs::ready()) {
      message = "Runtime, startup lock or already running protection";
      return false;
    }
    // Reserve durably BEFORE energizing. A later failed start consumes the event.
    if (!reserve(i, request["event_id"])) {
      message = "Event already reserved or storage write failed";
      return false;
    }
    bool ok = founderOutputs::pulseDevice(i, d.output, runtime);
    if (ok) {
      requested[i] = 100;
    }
    message = ok ? "Timed device started" : "Timed start rejected; event remains consumed";
    return ok;
  }
  if (strcmp(cmd, "set_device") || d.profile->timed) {
    message = "Use timed start/stop for this device";
    return false;
  }
  float value = 0;
  if (d.profile->analog) {
    if (!request["percent"].is<float>()) {
      message = "Analogue device requires percent";
      return false;
    }
    value = request["percent"];
  } else {
    if (!request["state"].is<bool>()) {
      message = "Switched device requires Boolean state";
      return false;
    }
    value = request["state"].as<bool>() ? 100 : 0;
  }
  if (!isfinite(value) || value < 0 || value > 100) {
    message = "Invalid device setpoint";
    return false;
  }
  bool ok = drive(i, value);
  if (ok) {
    requested[i] = value;
  }
  message = ok ? "Device applied" : "Device output rejected";
  return ok;
}
bool planEnabled(const char* id) {
  int i = slot(id);
  return i >= 0 && devices[i].profile && devices[i].plan.enabled;
}
bool hasPlans() {
  for (const auto& d : devices) {
    if (d.profile && d.plan.enabled) {
      return true;
    }
  }
  return false;
}
void loop() {
  const bool allowed = settingsReady() && !restMode_isEnabled() && !wifiOTA_requiresSetup() &&
                       !wifiOTA_isTesting() && !founderOutputs::inhibited() &&
                       founderOutputs::ready() && isTimeSynced();
  const unsigned minute = unsigned(getHour() * 60 + getMinute());
  const int date = getDateKey();
  for (unsigned i = 0; i < Count; ++i) {
    const auto& d = devices[i];
    if (!d.profile || !d.plan.enabled) {
      continue;
    }
    if (!allowed || minute >= 1440 || date < 0) {
      drive(i, 0);
      requested[i] = 0;
      continue;
    }
    if (!d.profile->timed) {
      unsigned value = founderSchedule::percent(d.plan, minute);
      if (founderOutputs::applied(d.output) != value) {
        if (drive(i, value)) {
          requested[i] = value;
        }
      }
      continue;
    }
    if (millis() < d.profile->startupLockMs || founderOutputs::applied(d.output) > 0) {
      continue;
    }
    for (unsigned n = 0; n < d.plan.count; ++n) {
      const auto& slot = d.plan.slots[n];
      const uint32_t token = founderSchedule::token(unsigned(date), minute);
      // Only the current scheduled minute, no accumulated or restart catch-up.
      if (slot.start != minute || token <= planLedger.token[i]) {
        continue;
      }
      PlanLedger next = planLedger;
      next.token[i] = token;
      next.checksum = planChecksum(next);
      // Reserve before any GPIO start. Failed start remains consumed.
      if (settings.putBytes("plan-ledger", &next, sizeof(next)) != sizeof(next)) {
        plansOk = false;
        stopAll();
        restMode_setEnabled(true, true);
        return;
      }
      planLedger = next;
      if (founderOutputs::pulseDevice(i, d.output, slot.runtimeMs)) {
        requested[i] = 100;
      }
    }
  }
}
void stopAll() {
  for (unsigned i = 0; i < Count; ++i) {
    if (devices[i].profile) {
      drive(i, 0);
      requested[i] = 0;
    }
  }
}
void appendJson(String& out) {
  JsonDocument root;
  root["equipment_settings_ok"] = settingsReady();
  root["equipment_revision"] = revision;
  auto fans = root["fan_ports"].to<JsonArray>();
  for (const auto& fan : fanPorts) {
    auto row = fans.add<JsonObject>();
    row["enabled"] = fan.enabled;
    row["name"] = fan.name;
  }
  auto catalog = root["device_catalog"].to<JsonArray>();
  unsigned count;
  const auto* all = profiles(count);
  for (unsigned i = 0; i < count; ++i) {
    auto row = catalog.add<JsonObject>();
    row["id"] = all[i].id;
    row["label"] = all[i].label;
    row["connection"] = all[i].analog ? "0_10v" : "dc_switch";
    row["timed"] = all[i].timed;
    row["optional_power"] = all[i].optionalPower;
    row["max_runtime_ms"] = all[i].maxRuntimeMs;
    row["startup_lock_ms"] = all[i].startupLockMs;
    auto protection = row["protections"].to<JsonArray>();
    protection.add("rest_setup_ota_off");
    protection.add("exclusive_output");
    protection.add("voltage_type_check");
    if (all[i].timed) {
      protection.add("timer_cutoff");
      protection.add("persistent_recent_event_ids");
    }
    if (all[i].startupLockMs) {
      protection.add("boot_startup_lock");
    }
    row["feedback_required"] = false;
    auto missing = row["missing_protections"].to<JsonArray>();
    missing.add("current_monitoring");
    missing.add("overtemperature_monitoring");
    if (all[i].kind == Kind::Pump) {
      missing.add("dry_run_detection");
      missing.add("flow_monitoring");
      missing.add("water_level_detection");
    }
    if (all[i].kind == Kind::SwitchedFan || all[i].kind == Kind::AnalogFan) {
      missing.add("tacho_feedback");
    }
    row["physical_acceptance"] = "OPEN";
    row["plan_kind"] = all[i].timed ? "pulse" : "windows";
    row["plan_max_slots"] = founderSchedule::MaxSlots;
  }
  auto caps = root["output_capabilities"].to<JsonArray>();
  for (int i = 0; i < 5; ++i) {
    auto row = caps.add<JsonObject>();
    row["id"] = channels()[i].id;
    row["kind"] = channels()[i].analog ? "0_10v" : "dc_switch";
    row["supply_voltage"] = channels()[i].voltage;
    row["max_current_a"] = nullptr;
    row["electrical_type"] = channels()[i].analog ? "0_10v_control" : "low_side_switch";
    if (channels()[i].analog) {
      row["control_min_v"] = 0;
      row["control_max_v"] = 10;
    }
  }
  auto rows = root["equipment"].to<JsonArray>();
  for (unsigned i = 0; i < Count; ++i) {
    if (devices[i].profile) {
      const auto& d = devices[i];
      auto row = rows.add<JsonObject>();
      char id[5];
      snprintf(id, sizeof(id), "dev%u", i + 1);
      row["id"] = id;
      row["name"] = d.name;
      row["profile"] = d.profile->id;
      row["output"] = channels()[d.output].id;
      appendPlan(row, d);
      if (!d.profile->analog) {
        row["voltage"] = d.voltage;
      }
      if (d.power >= 0) {
        row["power_output"] = channels()[d.power].id;
        row["power_voltage"] = d.voltage;
      }
      row["max_runtime_ms"] = d.maxRuntime;
      row["startup_lock_ms"] = d.profile->startupLockMs;
      row["startup_remaining_ms"] =
          millis() < d.profile->startupLockMs ? d.profile->startupLockMs - millis() : 0;
      row["requested_percent"] = requested[i];
      row["applied_percent"] = founderOutputs::applied(d.output);
      row["remaining_ms"] = founderOutputs::remainingMs(d.output);
      row["reason"] = founderOutputs::inhibited()             ? "OTA"
                      : restMode_isEnabled()                  ? "REST"
                      : !settingsReady()                      ? "STORAGE"
                      : !founderOutputs::ready()              ? "OUTPUT_INIT"
                      : !isTimeSynced() && d.plan.enabled     ? "WAIT_TIME"
                      : d.plan.enabled                        ? "SCHEDULE"
                      : founderOutputs::applied(d.output) > 0 ? "MANUAL"
                      : requested[i] > 0 && d.profile->timed  ? "RUNTIME_COMPLETE"
                                                              : "OFF";
    }
  }
  String json;
  serializeJson(root, json);
  json.remove(json.length() - 1);
  // Append object members into the existing top-level status.
  out += json.c_str() + 1;
}
}  // namespace founderEquipment
