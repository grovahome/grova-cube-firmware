#include "founder/event_log.h"

#include <ArduinoJson.h>

#include "founder/automation.h"
#include "founder/event_log_policy.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
namespace founderEvents {
static Recorder recorder;
static uint32_t epoch() {
  return isTimeSynced() ? getEpochSeconds() : 0;
}
void begin() {
  recorder.begin(millis(), epoch());
}
void configuration(uint32_t revision) {
  recorder.configuration(revision, millis(), epoch());
}
void operation(bool paused) {
  recorder.operation(paused, millis(), epoch());
}
void observe(unsigned device, const char* name, const founderAutomation::Config& config,
             bool enabled, bool canRun) {
  recorder.observe(device, name, config, enabled, canRun, millis(), epoch(),
                   founderAutomation::read);
}
void appendJson(String& out) {
  static const char* types[] = {
      "BOOT",          "CONFIGURATION_SAVED", "PAUSED",          "RELEASED",
      "INPUT_LOST",    "INPUT_RECOVERED",     "BELOW_MINIMUM",   "ABOVE_MAXIMUM",
      "LIMIT_CLEARED", "FALLBACK_REQUIRED",   "FALLBACK_ACTIVE", "FALLBACK_ENDED"};
  JsonDocument root;
  auto log = root["event_log"].to<JsonObject>();
  log["volatile"] = true;
  log["capacity"] = Capacity;
  log["count"] = recorder.count;
  log["total"] = recorder.total;
  log["overwritten"] = recorder.total - recorder.count;
  auto rows = log["events"].to<JsonArray>();
  for (unsigned n = 0; n < recorder.count; ++n) {
    const auto& e = recorder.newest(n);
    auto row = rows.add<JsonObject>();
    row["sequence"] = e.sequence;
    row["type"] = types[unsigned(e.type)];
    row["uptime_ms"] = e.uptimeMs;
    if (e.epoch) {
      row["epoch_s"] = e.epoch;
    } else {
      row["epoch_s"] = nullptr;
    }
    if (e.device >= 0) {
      char id[6];
      snprintf(id, sizeof(id), e.device < 5 ? "dev%u" : "FAN%u",
               e.device < 5 ? e.device + 1 : e.device - 4);
      row["device"] = id;
      row["name"] = e.name;
    }
    if (e.rule >= 0) {
      row["priority"] = e.rule + 1;
      row["source"] = e.source;
      row["signal"] = signalRegistry_name(static_cast<SignalId>(e.signal));
      row["unit"] = signalRegistry_unit(static_cast<SignalId>(e.signal));
    }
    if (e.hasLimit) {
      row["limit"] = e.limit;
    }
    if (e.hasValue) {
      row["value"] = e.value;
    } else {
      row["value"] = nullptr;
    }
    if (e.type == Type::InputLost) {
      row["input_state"] = e.inputState == 1 ? "MISSING" : e.inputState == 2 ? "STALE" : "INVALID";
    }
    if (e.type == Type::FallbackRequired || e.type == Type::FallbackActive) {
      row["percent"] = e.percent;
    }
    if (e.type == Type::ConfigurationSaved) {
      row["revision"] = e.revision;
    }
  }
  String json;
  serializeJson(root, json);
  json.remove(json.length() - 1);
  out += json.c_str() + 1;
}
}  // namespace founderEvents
