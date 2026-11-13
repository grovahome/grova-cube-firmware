#include "founder/history.h"

#include <ArduinoJson.h>

#include "founder/device_profiles.h"
#include "founder/devices.h"
#include "founder/history_policy.h"
#include "founder/outputs.h"
#include "modules/fan.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
namespace founderHistory {
static Recorder history;
void begin() {
  history.begin();
}
static void collect(const char* source, SignalId signal, float value, bool valid) {
  history.put(source, unsigned(signal), valid ? value : NAN);
}
void loop() {
  if (!history.start(millis(), isTimeSynced() ? getEpochSeconds() : 0)) {
    return;
  }
  for (unsigned n = 0; n < 5; ++n) {
    history.put(founderEquipment::channels()[n].id, SIGNAL_COUNT + n,
                founderOutputs::ready() ? founderOutputs::applied(n) : NAN);
  }
  history.put("FAN1", SIGNAL_COUNT + 5, getFanPercent());
  history.put("FAN2", SIGNAL_COUNT + 6, getFan2Percent());
  founderDevices::forEachSignal(collect);
}
String json() {
  unsigned used = 0;
  for (const auto& s : history.series) {
    if (s.used) {
      ++used;
    }
  }
  String out;
  if (!out.reserve(768 + history.count * 48 + used * (180 + history.count * 12))) {
    return String("{\"error\":\"HISTORY_MEMORY\"}");
  }
  out = "{\"volatile\":true,\"capacity\":";
  out += Capacity;
  out += ",\"interval_s\":30,\"max_series\":";
  out += MaxSeries;
  out += ",\"omitted\":";
  out += history.omitted;
  out += ",\"samples\":[";
  for (unsigned n = 0; n < history.count; ++n) {
    if (n) {
      out += ',';
    }
    const auto& t = history.stamps[history.position(n)];
    out += "{\"uptime_ms\":";
    out += t.uptime;
    out += ",\"epoch_s\":";
    if (t.epoch) {
      out += t.epoch;
    } else {
      out += "null";
    }
    out += '}';
  }
  out += "],\"series\":[";
  bool comma = false;
  for (const auto& s : history.series) {
    if (!s.used) {
      continue;
    }
    if (comma) {
      out += ',';
    }
    comma = true;
    JsonDocument meta;
    meta["source"] = s.source;
    meta["kind"] = s.metric < SIGNAL_COUNT ? "sensor" : "output";
    meta["signal"] = s.metric < SIGNAL_COUNT ? signalRegistry_name(static_cast<SignalId>(s.metric))
                                             : "actuation.percent";
    meta["unit"] =
        s.metric < SIGNAL_COUNT ? signalRegistry_unit(static_cast<SignalId>(s.metric)) : "%";
    String header;
    serializeJson(meta, header);
    header.remove(header.length() - 1);
    out += header;
    out += ",\"values\":[";
    for (unsigned n = 0; n < history.count; ++n) {
      if (n) {
        out += ',';
      }
      const float v = s.values[history.position(n)];
      out += isfinite(v) ? String(v, 3) : String("null");
    }
    out += "]}";
  }
  out += "]}";
  return out;
}
}  // namespace founderHistory
