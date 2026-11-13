#include "founder/equipment.h"

#include <Preferences.h>
#include <stddef.h>

#include <memory>
#include <new>

#include "firmware_info.h"
#include "founder/automation.h"
#include "founder/device_profiles.h"
#include "founder/event_log.h"
#include "founder/history.h"
#include "founder/outputs.h"
#include "founder/schedule_policy.h"
#include "modules/fan.h"
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
  bool automatic = false;
  uint8_t role = 0;
  founderAutomation::Config automation;
};
static Device devices[Count];
static bool cleanText(const char*, size_t);
struct FanPort {
  bool enabled = false;
  char name[40] = {};
  bool automatic = false;
  uint8_t role = 0;
  founderAutomation::Config automation;
  founderSchedule::Plan plan;
};
static unsigned fanManual[2] = {};
static founderAutomation::State deviceAutomationState[Count], fanAutomationState[2];
static founderAutomation::Decision deviceDecision[Count], fanDecision[2];
static bool parseAutomation(JsonObjectConst row, founderAutomation::Config& next, uint8_t& role,
                            bool analog, bool pulse, unsigned maximum, String& error) {
  if (!row["fan_role"].isNull()) {
    if (!row["fan_role"].is<const char*>()) {
      return false;
    }
    const char* value = row["fan_role"];
    if (!strcmp(value, "circulation")) {
      role = 0;
    } else if (!strcmp(value, "intake")) {
      role = 1;
    } else if (!strcmp(value, "exhaust")) {
      role = 2;
    } else {
      error = "Invalid fan function";
      return false;
    }
  }
  return founderAutomation::parse(row["automation"], next, analog, pulse, maximum, error);
}
static const char* roleName(unsigned role) {
  return role == 1 ? "intake" : role == 2 ? "exhaust" : "circulation";
}
static void appendAutomation(JsonObject row, const founderAutomation::Config& config, uint8_t role,
                             bool fan, bool pulse) {
  founderAutomation::append(row, config);
  if (fan) {
    row["fan_role"] = roleName(role);
  }
  if (pulse) {
    row["automation"]["pulse_ms"] = config.pulseMs;
    row["automation"]["cooldown_s"] = config.cooldownSeconds;
  }
}
static bool parsePlan(JsonVariantConst, Device&, String&);
static void appendPlan(JsonObject, const Device&);
static bool parseMode(JsonVariantConst value, bool& automatic, String& error) {
  if (value.isNull()) {
    return true;
  }
  if (!value.is<const char*>() ||
      (strcmp(value, "manual") && strcmp(value, "scheduled") && strcmp(value, "automatic"))) {
    error = "Expected manual or automatic fan mode";
    return false;
  }
  automatic = !strcmp(value, "automatic");
  return true;
}
struct WorkingConfiguration {
  Device devices[Count];
  FanPort fans[2];
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
    next[i].automatic = false;
    next[i].automation = founderAutomation::Config{};
    next[i].role = 0;
    if (!parseAutomation(row.as<JsonObjectConst>(), next[i].automation, next[i].role, true, false,
                         0, error)) {
      return false;
    }
    Device temporary;
    temporary.profile = profile("fan_0_10v");
    if (!parseMode(row["mode"], next[i].automatic, error) ||
        !parsePlan(row["plan"], temporary, error)) {
      return false;
    }
    next[i].plan = temporary.plan;
    if (next[i].automatic && next[i].plan.enabled) {
      error = "Automatic fans cannot use a manual daily plan";
      return false;
    }
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
enum class RunState : uint32_t {
  None,
  Reserved,
  Started,
  Completed,
  InterruptedUnknown,
  StartFailed,
  Skipped,
  LegacyUnknown,
  Interrupted
};
struct RunRecord {
  uint32_t token = 0, state = 0, requestedMs = 0, elapsedMs = 0;
  uint32_t commandStartedAt = 0, recordedAt = 0, output = 0;
  char name[40] = {};
};
struct LegacyPlanLedger {
  uint32_t schema = 1, token[Count] = {}, checksum = 0;
};
struct PlanLedger {
  uint32_t schema = 2, token[Count] = {};
  RunRecord last[Count];
  uint32_t checksum = 0;
};
static PlanLedger planLedger;
static bool plansOk = true, activeRuns[Count] = {};
static uint32_t activeAt[Count] = {};
static uint32_t bytesChecksum(const void* ptr, size_t length) {
  uint32_t h = 2166136261UL;
  const auto* b = static_cast<const uint8_t*>(ptr);
  for (size_t n = 0; n < length; ++n) {
    h = (h ^ b[n]) * 16777619UL;
  }
  return h;
}
static uint32_t planChecksum(const PlanLedger& l) {
  return bytesChecksum(&l, offsetof(PlanLedger, checksum));
}
static bool savePlanLedger(PlanLedger next) {
  next.checksum = planChecksum(next);
  if (settings.putBytes("plan-ledger", &next, sizeof(next)) != sizeof(next)) {
    plansOk = false;
    return false;
  }
  planLedger = next;
  return true;
}
static const char* runStateName(uint32_t state) {
  static const char* names[] = {
      "NONE",         "RESERVED", "STARTED",        "COMPLETED",  "INTERRUPTED_UNKNOWN",
      "START_FAILED", "SKIPPED",  "LEGACY_UNKNOWN", "INTERRUPTED"};
  return state <= unsigned(RunState::Interrupted) ? names[state] : "INVALID";
}
static bool finishRun(unsigned i, RunState state) {
  PlanLedger next = planLedger;
  auto& record = next.last[i];
  record.state = unsigned(state);
  if (getEpochSeconds() > record.recordedAt) {
    record.recordedAt = getEpochSeconds();
  }
  const uint32_t elapsed = uint32_t(millis() - activeAt[i]);
  record.elapsedMs = elapsed < record.requestedMs ? elapsed : record.requestedMs;
  activeRuns[i] = false;
  return savePlanLedger(next);
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
  if (!value["since_token"].isNull()) {
    if (!value["since_token"].is<uint32_t>()) {
      error = "Invalid plan baseline";
      return false;
    }
    plan.sinceToken = value["since_token"];
  }
  if (!value["catch_up_minutes"].isNull()) {
    if (!device.profile->timed || !value["catch_up_minutes"].is<unsigned>() ||
        value["catch_up_minutes"].as<unsigned>() > 60) {
      error = "Catch-up applies only to pulse devices, 0..60 minutes";
      return false;
    }
    plan.catchUpMinutes = value["catch_up_minutes"];
  }
  if (!value["interval"].isNull()) {
    auto v = value["interval"];
    auto& i = plan.interval;
    if (!v.is<JsonObjectConst>() || !v["every_minutes"].is<unsigned>() ||
        v["every_minutes"].as<unsigned>() > 1440 || !v["anchor_minute"].is<unsigned>() ||
        v["anchor_minute"].as<unsigned>() >= 1440) {
      error = "Invalid repeating interval";
      return false;
    }
    i.enabled = true;
    i.everyMinutes = v["every_minutes"];
    i.anchor = v["anchor_minute"];
    if (device.profile->timed) {
      if (!v["runtime_ms"].is<unsigned>() || v["runtime_ms"].as<unsigned>() > device.maxRuntime) {
        error = "Invalid interval pulse";
        return false;
      }
      i.runtimeMs = v["runtime_ms"];
    } else {
      if (!v["duration_minutes"].is<unsigned>() || v["duration_minutes"].as<unsigned>() > 1440 ||
          !v["percent"].is<unsigned>() || v["percent"].as<unsigned>() > 100 ||
          (!device.profile->analog && v["percent"].as<unsigned>() != 100)) {
        error = "Invalid interval duration/percent";
        return false;
      }
      i.durationMinutes = v["duration_minutes"];
      i.percent = v["percent"];
    }
  }
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
  if (device.profile->timed) {
    p["catch_up_minutes"] = device.plan.catchUpMinutes;
    p["since_token"] = device.plan.sinceToken;
  }
  if (device.plan.interval.enabled) {
    const auto& i = device.plan.interval;
    auto v = p["interval"].to<JsonObject>();
    v["anchor_minute"] = i.anchor;
    v["every_minutes"] = i.everyMinutes;
    if (device.profile->timed) {
      v["runtime_ms"] = i.runtimeMs;
    } else {
      v["duration_minutes"] = i.durationMinutes;
      v["percent"] = i.percent;
    }
  }
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
    if (p->timed) {
      device.automation.pulseMs = device.maxRuntime < 1000 ? device.maxRuntime : 1000;
    }
    const bool fan = p->kind == Kind::SwitchedFan || p->kind == Kind::AnalogFan;
    if (!parseMode(row["mode"], device.automatic, error) || (!fan && !row["fan_role"].isNull()) ||
        !parseAutomation(row, device.automation, device.role, p->analog, p->timed,
                         device.maxRuntime, error)) {
      return false;
    }
    if (!parsePlan(row["plan"], device, error)) {
      return false;
    }
    if (device.automatic && device.plan.enabled) {
      error = "Automatic fans cannot use a manual daily plan";
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
    deviceAutomationState[i] = {};
    deviceDecision[i] = {};
  }
}
void begin() {
  founderEvents::begin();
  founderHistory::begin();
  fanManual[0] = fanManual[1] = 0;
  for (unsigned n = 0; n < 2; ++n) {
    fanAutomationState[n] = {};
    fanDecision[n] = {};
  }
  for (unsigned n = 1; n <= 2; ++n) {
    fan_setDemand(n, 0);
  }
  planLedger = PlanLedger{};
  plansOk = true;
  for (unsigned i = 0; i < Count; ++i) {
    activeRuns[i] = false;
  }
  std::unique_ptr<WorkingConfiguration> working(new (std::nothrow) WorkingConfiguration);
  if (!working) {
    storage = configOk = eventsOk = false;
    for (auto& d : devices) {
      d = Device{};
    }
    int8_t owners[5] = {-1, -1, -1, -1, -1};
    bool pulse[5] = {};
    founderOutputs::assignOwners(owners, pulse);
    return;
  }
  auto& initial = working->devices;
  auto& initialFans = working->fans;
  apply(initial);
  revision = 0;
  fanPorts[0] = FanPort{};
  fanPorts[1] = FanPort{};
  strcpy(fanPorts[0].name, "Lüfter 1");
  strcpy(fanPorts[1].name, "Lüfter 2");
  initialFans[0] = fanPorts[0];
  initialFans[1] = fanPorts[1];
  storage = settings.begin("founder-devices", false);
  configOk = storage;
  eventsOk = storage;
  memset(&ledger, 0, sizeof(ledger));
  ledger.schema = 1;
  if (storage && settings.isKey("configuration")) {
    size_t size = settings.getBytesLength("configuration");
    std::unique_ptr<char[]> bytes(new (std::nothrow) char[(founderAutomation::ConfigMaxBytes + 1)]);
    JsonDocument root;
    String error;
    configOk = bool(bytes) && size > 0 && size <= founderAutomation::ConfigMaxBytes &&
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
      for (auto& d : initial) {
        d = Device{};
      }
      apply(initial);
    }
  }
  if (configOk) {
    apply(initial);
    fanPorts[0] = initialFans[0];
    fanPorts[1] = initialFans[1];
  }
  if (storage && settings.isKey("plan-ledger")) {
    bool rewrite = false;
    if (settings.getBytesLength("plan-ledger") == sizeof(LegacyPlanLedger)) {
      LegacyPlanLedger old;
      plansOk = settings.getBytes("plan-ledger", &old, sizeof(old)) == sizeof(old) &&
                old.schema == 1 &&
                old.checksum == bytesChecksum(&old, offsetof(LegacyPlanLedger, checksum));
      if (plansOk) {
        for (unsigned i = 0; i < Count; ++i) {
          planLedger.token[i] = old.token[i];
          planLedger.last[i].token = old.token[i];
          if (old.token[i]) {
            planLedger.last[i].state = unsigned(RunState::LegacyUnknown);
          }
        }
        rewrite = true;
      }
    } else {
      PlanLedger saved;
      plansOk = settings.getBytesLength("plan-ledger") == sizeof(saved) &&
                settings.getBytes("plan-ledger", &saved, sizeof(saved)) == sizeof(saved) &&
                saved.schema == 2 && saved.checksum == planChecksum(saved);
      if (plansOk) {
        for (unsigned i = 0; i < Count; ++i) {
          const auto& r = saved.last[i];
          if (r.state > unsigned(RunState::Interrupted) || r.token != saved.token[i] ||
              r.name[39] != 0 || r.output >= 5 || r.requestedMs > 10000 ||
              r.elapsedMs > r.requestedMs ||
              (r.state == unsigned(RunState::None) && r.token != 0) ||
              (r.state == unsigned(RunState::Completed) &&
               (!r.requestedMs || r.elapsedMs != r.requestedMs || !r.commandStartedAt))) {
            plansOk = false;
          }
        }
        if (plansOk) {
          planLedger = saved;
        }
      }
    }
    if (plansOk) {
      for (auto& r : planLedger.last) {
        if (r.state == unsigned(RunState::Reserved) || r.state == unsigned(RunState::Started)) {
          // Start-before-power-loss cannot prove a full run. Never report completion.
          r.state = unsigned(RunState::InterruptedUnknown);
          if (isTimeSynced() && getEpochSeconds() > r.recordedAt) {
            r.recordedAt = getEpochSeconds();
          }
          rewrite = true;
        }
      }
      if (rewrite) {
        savePlanLedger(planLedger);
      }
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
static void serializeConfiguration(const Device (&next)[Count], const FanPort (&nextFans)[2],
                                   uint32_t version, String& bytes) {
  JsonDocument root;
  root["schema"] = 1;
  root["revision"] = version;
  auto fans = root["fan_ports"].to<JsonArray>();
  for (const auto& fan : nextFans) {
    auto row = fans.add<JsonObject>();
    row["enabled"] = fan.enabled;
    row["name"] = fan.name;
    row["mode"] = fan.automatic ? "automatic" : fan.plan.enabled ? "scheduled" : "manual";
    row["automation_ready"] = fan.automation.count > 0;
    appendAutomation(row, fan.automation, fan.role, true, false);
    Device temporary;
    temporary.profile = profile("fan_0_10v");
    temporary.plan = fan.plan;
    appendPlan(row, temporary);
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
      row["mode"] = next[i].automatic ? "automatic" : next[i].plan.enabled ? "scheduled" : "manual";
      appendAutomation(
          row, next[i].automation, next[i].role,
          next[i].profile->kind == Kind::SwitchedFan || next[i].profile->kind == Kind::AnalogFan,
          next[i].profile->timed);
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
  serializeJson(root, bytes);
}
bool backup(String& output) {
  if (!storage || !configOk) {
    return false;
  }
  String bytes;
  serializeConfiguration(devices, fanPorts, revision, bytes);
  JsonDocument config;
  if (deserializeJson(config, bytes)) {
    return false;
  }
  for (JsonObject f : config["fan_ports"].as<JsonArray>()) {
    f.remove("automation_ready");
  }
  for (JsonObject e : config["equipment"].as<JsonArray>()) {
    if (e["plan"].is<JsonObject>()) {
      e["plan"].as<JsonObject>().remove("since_token");
    }
  }
  JsonDocument root;
  root["format"] = "grova-core-equipment";
  root["version"] = 1;
  root["hardware_profile"] = GROVA_BOARD_PROFILE_ID;
  root["firmware_version"] = GROVA_FIRMWARE_VERSION;
  if (isTimeSynced()) {
    root["created_at_s"] = getEpochSeconds();
  } else {
    root["created_at_s"] = nullptr;
  }
  root["configuration"].set(config.as<JsonObject>());
  output = "";
  serializeJson(root, output);
  return !root.overflowed() && output.length() <= BackupMaxBytes;
}
bool restoreBackup(JsonObjectConst request, String& message, bool validateOnly) {
  const auto b = request["backup"];
  const auto c = b["configuration"];
  if (!request["expected_revision"].is<uint32_t>() || !b.is<JsonObjectConst>() ||
      !b["format"].is<const char*>() || strcmp(b["format"], "grova-core-equipment") ||
      !b["version"].is<unsigned>() || b["version"].as<unsigned>() != 1 ||
      !b["hardware_profile"].is<const char*>() ||
      strcmp(b["hardware_profile"], GROVA_BOARD_PROFILE_ID) || !c.is<JsonObjectConst>() ||
      !c["schema"].is<unsigned>() || c["schema"].as<unsigned>() != 1 ||
      !c["fan_ports"].is<JsonArrayConst>()) {
    message = "Invalid backup format, version or hardware profile";
    return false;
  }
  JsonDocument next;
  next["expected_revision"] = request["expected_revision"];
  next["equipment"].set(c["equipment"]);
  next["fan_ports"].set(c["fan_ports"]);
  if (next.overflowed()) {
    message = "Not enough memory for backup";
    return false;
  }
  return configure(next.as<JsonObjectConst>(), message, validateOnly);
}
bool configure(JsonObjectConst request, String& message, bool validateOnly) {
  if (!storage || (!validateOnly && !restMode_isEnabled())) {
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
  std::unique_ptr<WorkingConfiguration> working(new (std::nothrow) WorkingConfiguration);
  if (!working) {
    message = "Not enough memory for device configuration";
    return false;
  }
  auto& next = working->devices;
  auto& nextFans = working->fans;
  nextFans[0] = fanPorts[0];
  nextFans[1] = fanPorts[1];
  if (!parseFans(request, nextFans, message) || !parse(request, next, message)) {
    return false;
  }
  for (unsigned i = 0; i < Count; ++i) {
    if (!next[i].profile || !next[i].profile->timed) {
      continue;
    }
    const auto& old = devices[i];
    bool unchanged = old.profile == next[i].profile && old.output == next[i].output &&
                     old.maxRuntime == next[i].maxRuntime &&
                     old.plan.enabled == next[i].plan.enabled &&
                     old.plan.count == next[i].plan.count &&
                     founderSchedule::same(old.plan.interval, next[i].plan.interval);
    for (unsigned n = 0; unchanged && n < next[i].plan.count; ++n) {
      unchanged = old.plan.slots[n].start == next[i].plan.slots[n].start &&
                  old.plan.slots[n].runtimeMs == next[i].plan.slots[n].runtimeMs;
    }
    next[i].plan.sinceToken =
        unchanged ? old.plan.sinceToken
                  : (isTimeSynced() && getDateKey() >= 0
                         ? founderSchedule::token(getDateKey(), getHour() * 60 + getMinute())
                         : UINT32_MAX);
  }
  String bytes;
  serializeConfiguration(next, nextFans, revision + 1, bytes);
  if (bytes.length() > founderAutomation::ConfigMaxBytes) {
    message = "Device configuration exceeds storage limit";
    return false;
  }
  if (validateOnly) {
    message = "Configuration validated";
    return true;
  }
  if (settings.putBytes("configuration", bytes.c_str(), bytes.length()) != bytes.length()) {
    message = "Device assignment was not saved; previous assignment remains";
    return false;
  }
  apply(next);
  fanManual[0] = fanManual[1] = 0;
  for (unsigned n = 0; n < 2; ++n) {
    fanAutomationState[n] = {};
    fanDecision[n] = {};
  }
  fan_setDemand(1, 0);
  fan_setDemand(2, 0);
  fanPorts[0] = nextFans[0];
  fanPorts[1] = nextFans[1];
  ++revision;
  founderEvents::configuration(revision);
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
  if (d.plan.enabled || d.automatic) {
    message = "Device is automated; choose manual mode or pause automation";
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
  return i >= 0 && devices[i].profile && (devices[i].plan.enabled || devices[i].automatic);
}
bool setFanManual(unsigned fan, unsigned percent) {
  if (fan < 1 || fan > 2 || percent > 100) {
    return false;
  }
  const auto& port = fanPorts[fan - 1];
  if (percent && (!fanEnabled(fan) || port.automatic || port.plan.enabled || restMode_isEnabled() ||
                  !founderOutputs::ready())) {
    return false;
  }
  fanManual[fan - 1] = percent;
  return fan_setDemand(fan, percent);
}
bool fanControlled(unsigned fan) {
  return fan >= 1 && fan <= 2 && (fanPorts[fan - 1].automatic || fanPorts[fan - 1].plan.enabled);
}
bool hasPlans() {
  for (const auto& fan : fanPorts) {
    if (fan.enabled && (fan.plan.enabled || (fan.automatic && fan.automation.count))) {
      return true;
    }
  }
  for (const auto& d : devices) {
    if (d.profile && (d.plan.enabled || (d.automatic && d.automation.count))) {
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
  for (unsigned n = 0; n < 2; ++n) {
    const auto& fan = fanPorts[n];
    unsigned value = 0;
    if (fanEnabled(n + 1) && !restMode_isEnabled() && !wifiOTA_requiresSetup() &&
        !wifiOTA_isTesting() && !founderOutputs::inhibited()) {
      if (fan.automatic) {
        fanDecision[n] = founderAutomation::evaluate(fan.automation, fanAutomationState[n],
                                                     millis(), true, founderAutomation::read);
        value = allowed ? fanDecision[n].percent : 0;
      } else {
        value = fan.plan.enabled
                    ? (allowed && minute < 1440 ? founderSchedule::percent(fan.plan, minute) : 0)
                    : fanManual[n];
      }
    }
    fan_setDemand(n + 1, value);
  }
  if (allowed && minute < 1440 && date >= 0) {
    bool baselineChanged = false;
    for (auto& d : devices) {
      if (d.profile && d.profile->timed && d.plan.enabled && d.plan.sinceToken == UINT32_MAX) {
        d.plan.sinceToken = founderSchedule::token(date, minute);
        baselineChanged = true;
      }
    }
    if (baselineChanged) {
      String bytes;
      serializeConfiguration(devices, fanPorts, revision, bytes);
      if (bytes.length() > founderAutomation::ConfigMaxBytes ||
          settings.putBytes("configuration", bytes.c_str(), bytes.length()) != bytes.length()) {
        plansOk = false;
        stopAll();
        restMode_setEnabled(true, true);
        return;
      }
    }
  }
  for (unsigned i = 0; i < Count; ++i) {
    const auto& d = devices[i];
    if (activeRuns[i]) {
      const bool on = d.profile && founderOutputs::applied(d.output) > 0;
      if (!allowed || !on) {
        const bool complete =
            allowed && !on && uint32_t(millis() - activeAt[i]) >= planLedger.last[i].requestedMs;
        if (!finishRun(i, complete ? RunState::Completed : RunState::Interrupted)) {
          stopAll();
          restMode_setEnabled(true, true);
          return;
        }
      }
    }
    const bool sensor = d.profile && d.automatic;
    unsigned sensorDemand = 0;
    if (sensor) {
      deviceDecision[i] =
          founderAutomation::evaluate(d.automation, deviceAutomationState[i], millis(),
                                      d.profile->analog, founderAutomation::read);
      sensorDemand = deviceDecision[i].percent;
      if (d.profile->timed && !sensorDemand) {
        drive(i, 0);
        requested[i] = 0;
      }
      if (!d.profile->timed) {
        drive(i, allowed ? sensorDemand : 0);
        requested[i] = allowed ? sensorDemand : 0;
        continue;
      }
    }
    if (!d.profile || (!d.plan.enabled && !sensor)) {
      continue;
    }
    if (!allowed || minute >= 1440 || date < 0) {
      drive(i, 0);
      requested[i] = 0;
      continue;
    }
    if (!d.profile->timed) {
      const unsigned value = founderSchedule::percent(d.plan, minute);
      if (founderOutputs::applied(d.output) != value && drive(i, value)) {
        requested[i] = value;
      }
      continue;
    }
    if (millis() < d.profile->startupLockMs || founderOutputs::applied(d.output) > 0) {
      continue;
    }
    if (sensor && (!sensorDemand || (planLedger.last[i].recordedAt &&
                                     (getEpochSeconds() < planLedger.last[i].recordedAt ||
                                      getEpochSeconds() - planLedger.last[i].recordedAt <
                                          d.automation.cooldownSeconds)))) {
      continue;
    }
    auto due = founderSchedule::latest(d.plan, unsigned(date), minute);
    if (sensor) {
      due.token = founderSchedule::token(unsigned(date), minute);
      due.slot = 0;
      due.ageMinutes = 0;
    }
    if (!due.token || (!sensor && due.token < d.plan.sinceToken) ||
        due.token <= planLedger.token[i]) {
      continue;
    }
    founderSchedule::Slot slot;
    if (sensor) {
      slot.runtimeMs = d.automation.pulseMs;
    } else if (d.plan.interval.enabled) {
      slot.runtimeMs = d.plan.interval.runtimeMs;
    } else {
      slot = d.plan.slots[due.slot];
    }
    PlanLedger next = planLedger;
    next.token[i] = due.token;
    next.last[i] = RunRecord{};
    auto& record = next.last[i];
    record.token = due.token;
    record.requestedMs = slot.runtimeMs;
    record.output = d.output;
    strcpy(record.name, d.name);
    record.recordedAt = getEpochSeconds();
    const bool eligible = sensor || due.ageMinutes <= d.plan.catchUpMinutes;
    record.state = unsigned(eligible ? RunState::Reserved : RunState::Skipped);
    // Consume only the latest occurrence; never replay a queue of missed doses.
    if (!savePlanLedger(next)) {
      stopAll();
      restMode_setEnabled(true, true);
      return;
    }
    if (!eligible) {
      continue;
    }
    activeAt[i] = millis();
    const bool started = founderOutputs::pulseDevice(i, d.output, slot.runtimeMs);
    next = planLedger;
    next.last[i].state = unsigned(started ? RunState::Started : RunState::StartFailed);
    next.last[i].commandStartedAt = started ? getEpochSeconds() : 0;
    next.last[i].recordedAt = getEpochSeconds();
    activeRuns[i] = started;
    if (!savePlanLedger(next)) {
      stopAll();
      restMode_setEnabled(true, true);
      return;
    }
    if (started) {
      requested[i] = 100;
    }
  }
}
void stopAll() {
  for (unsigned n = 0; n < 2; ++n) {
    fanManual[n] = 0;
    fan_setDemand(n + 1, 0);
  }
  // Physical OFF precedes history writes, which may take time or fail.
  for (unsigned i = 0; i < Count; ++i) {
    if (devices[i].profile) {
      drive(i, 0);
      requested[i] = 0;
    }
  }
  for (unsigned i = 0; i < Count; ++i) {
    if (activeRuns[i]) {
      if (plansOk) {
        finishRun(i, RunState::Interrupted);
      } else {
        activeRuns[i] = false;
      }
    }
  }
  if (!plansOk) {
    restMode_setEnabled(true, true);
  }
}
static const char* automationBlock() {
  if (founderOutputs::inhibited()) {
    return "OTA";
  }
  if (wifiOTA_requiresSetup() || wifiOTA_isTesting()) {
    return "SETUP";
  }
  if (restMode_isEnabled()) {
    return "REST";
  }
  if (!settingsReady()) {
    return "STORAGE";
  }
  if (!founderOutputs::ready()) {
    return "OUTPUT_INIT";
  }
  if (!isTimeSynced()) {
    return "WAIT_TIME";
  }
  return nullptr;
}
void recordEvents() {
  founderEvents::operation(restMode_isEnabled());
  const bool allowed = !automationBlock();
  for (unsigned n = 0; n < Count; ++n) {
    const auto& d = devices[n];
    founderEvents::observe(n, d.name, d.automation, d.profile && d.automatic,
                           allowed && (!d.profile || millis() >= d.profile->startupLockMs));
  }
  for (unsigned n = 0; n < 2; ++n) {
    const auto& f = fanPorts[n];
    founderEvents::observe(Count + n, f.name, f.automation, f.enabled && f.automatic,
                           allowed && !fan_getTachoFault(n + 1));
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
    row["mode"] = fan.automatic ? "automatic" : fan.plan.enabled ? "scheduled" : "manual";
    row["automation_ready"] = fan.automation.count > 0;
    appendAutomation(row, fan.automation, fan.role, true, false);
    if (fan.automatic) {
      const unsigned n = &fan - fanPorts;
      const char* block = automationBlock();
      if (!block && !fan.enabled) {
        block = "DISABLED";
      }
      if (!block && fan_getTachoFault(n + 1)) {
        block = "TACHO_FAULT";
      }
      founderAutomation::appendDecision(row, fanDecision[n], fan.automation, block);
    }
    Device temporary;
    temporary.profile = profile("fan_0_10v");
    temporary.plan = fan.plan;
    appendPlan(row, temporary);
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
      row["mode"] = d.automatic ? "automatic" : d.plan.enabled ? "scheduled" : "manual";
      row["automation_ready"] = d.automation.count > 0;
      appendAutomation(row, d.automation, d.role,
                       d.profile->kind == Kind::SwitchedFan || d.profile->kind == Kind::AnalogFan,
                       d.profile->timed);
      if (d.automatic) {
        const char* block = automationBlock();
        if (!block && millis() < d.profile->startupLockMs) {
          block = "STARTUP_LOCK";
        }
        founderAutomation::appendDecision(row, deviceDecision[i], d.automation, block);
        if (d.profile->timed) {
          const uint32_t at = planLedger.last[i].recordedAt;
          const uint32_t elapsed = getEpochSeconds() >= at ? getEpochSeconds() - at : 0;
          row["automation_status"]["cooldown_remaining_s"] =
              at && elapsed < d.automation.cooldownSeconds ? d.automation.cooldownSeconds - elapsed
                                                           : 0;
        }
      }
      appendPlan(row, d);
      if (d.profile->timed) {
        const auto& r = planLedger.last[i];
        auto last = row["last_run"].to<JsonObject>();
        last["state"] = runStateName(r.state);
        last["scheduled_date_key"] = r.token / 1440;
        last["scheduled_minute"] = r.token % 1440;
        last["requested_ms"] = r.requestedMs;
        if (r.state == unsigned(RunState::Completed) ||
            r.state == unsigned(RunState::Interrupted) || r.state == unsigned(RunState::Skipped) ||
            r.state == unsigned(RunState::StartFailed)) {
          last["command_elapsed_ms"] = r.elapsedMs;
        } else {
          last["command_elapsed_ms"] = nullptr;
        }
        last["durable"] = plansOk;
        last["command_started_at_s"] = r.commandStartedAt;
        last["recorded_at_s"] = r.recordedAt;
        last["name"] = r.name;
        if (r.state == unsigned(RunState::None) || r.state == unsigned(RunState::LegacyUnknown)) {
          last["output"] = nullptr;
        } else {
          last["output"] = channels()[r.output].id;
        }
        last["water_delivery_verified"] = false;
      }
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
      row["reason"] = founderOutputs::inhibited()                          ? "OTA"
                      : restMode_isEnabled()                               ? "REST"
                      : !settingsReady()                                   ? "STORAGE"
                      : !founderOutputs::ready()                           ? "OUTPUT_INIT"
                      : !isTimeSynced() && (d.plan.enabled || d.automatic) ? "WAIT_TIME"
                      : d.plan.enabled                                     ? "SCHEDULE"
                      : founderOutputs::applied(d.output) > 0              ? "MANUAL"
                      : requested[i] > 0 && d.profile->timed               ? "RUNTIME_COMPLETE"
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
