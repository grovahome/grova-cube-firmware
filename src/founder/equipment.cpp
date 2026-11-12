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
  record.recordedAt = getEpochSeconds();
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
  for (unsigned i = 0; i < Count; ++i) {
    activeRuns[i] = false;
  }
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
          r.recordedAt = getEpochSeconds();
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
  serializeJson(root, bytes);
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
  for (unsigned i = 0; i < Count; ++i) {
    if (!next[i].profile || !next[i].profile->timed) {
      continue;
    }
    const auto& old = devices[i];
    bool unchanged = old.profile == next[i].profile && old.output == next[i].output &&
                     old.maxRuntime == next[i].maxRuntime &&
                     old.plan.enabled == next[i].plan.enabled &&
                     old.plan.count == next[i].plan.count;
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
      if (bytes.length() > 4096 ||
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
    if (!d.profile || !d.plan.enabled) {
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
    const auto due = founderSchedule::latest(d.plan, unsigned(date), minute);
    if (!due.token || due.token < d.plan.sinceToken || due.token <= planLedger.token[i]) {
      continue;
    }
    const auto& slot = d.plan.slots[due.slot];
    PlanLedger next = planLedger;
    next.token[i] = due.token;
    next.last[i] = RunRecord{};
    auto& record = next.last[i];
    record.token = due.token;
    record.requestedMs = slot.runtimeMs;
    record.output = d.output;
    strcpy(record.name, d.name);
    record.recordedAt = getEpochSeconds();
    const bool eligible = due.ageMinutes <= d.plan.catchUpMinutes;
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
