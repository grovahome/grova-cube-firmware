#include "founder/automation.h"

#include "founder/devices.h"
#include "modules/signal_registry.h"
namespace founderAutomation {
static bool number(JsonVariantConst v) {
  return v.is<float>() && isfinite(v.as<float>());
}
bool parse(JsonVariantConst value, Config& next, bool analog, bool pulse, unsigned maxRuntime,
           String& error) {
  if (value.isNull()) {
    return true;
  }
  auto fail = [&]() {
    error =
        "Invalid sensor automation: check sources, targets, limits, hysteresis, actions and pulse "
        "protection";
    return false;
  };
  if (!value.is<JsonObjectConst>() || !value["rules"].is<JsonArrayConst>() ||
      value["rules"].size() > MaxRules) {
    return fail();
  }
  if (!value["fallback_percent"].isNull()) {
    if (!value["fallback_percent"].is<unsigned>() ||
        value["fallback_percent"].as<unsigned>() > 100) {
      return fail();
    }
    next.fallbackPercent = value["fallback_percent"];
  }
  if (pulse) {
    if (!value["pulse_ms"].isNull()) {
      if (!value["pulse_ms"].is<unsigned>() || value["pulse_ms"].as<unsigned>() > maxRuntime) {
        return fail();
      }
      next.pulseMs = value["pulse_ms"];
    }
    if (!value["cooldown_s"].isNull()) {
      if (!value["cooldown_s"].is<uint32_t>()) {
        return fail();
      }
      next.cooldownSeconds = value["cooldown_s"];
    }
  } else if (!value["pulse_ms"].isNull() || !value["cooldown_s"].isNull()) {
    return fail();
  }
  for (JsonObjectConst item : value["rules"].as<JsonArrayConst>()) {
    auto& r = next.rules[next.count++];
    SignalId id;
    if (!item["source"].is<const char*>() || !item["signal"].is<const char*>() ||
        !signalRegistry_idFromName(item["signal"], id)) {
      return fail();
    }
    const char* source = item["source"];
    size_t len = strlen(source);
    if (!len || len >= sizeof(r.source)) {
      return fail();
    }
    for (const char* p = source; *p; ++p) {
      if (uint8_t(*p) < 32) {
        return fail();
      }
    }
    strcpy(r.source, source);
    r.signal = id;
    if (!number(item["target"]) || !item["direction"].is<const char*>()) {
      return fail();
    }
    const char* direction = item["direction"];
    if (strcmp(direction, "raise") && strcmp(direction, "lower")) {
      return fail();
    }
    r.raise = !strcmp(direction, "raise");
    r.target = item["target"];
    if (!item["band"].isNull()) {
      if (!number(item["band"])) {
        return fail();
      }
      r.band = item["band"];
    }
    if (!item["hysteresis"].isNull()) {
      if (!number(item["hysteresis"])) {
        return fail();
      }
      r.hysteresis = item["hysteresis"];
    }
    if (!item["delay_s"].isNull()) {
      if (!item["delay_s"].is<unsigned>() || item["delay_s"].as<unsigned>() > 300) {
        return fail();
      }
      r.delaySeconds = item["delay_s"];
    }
    r.hasMinimum = !item["minimum"].isNull();
    r.hasMaximum = !item["maximum"].isNull();
    if (r.hasMinimum) {
      if (!number(item["minimum"])) {
        return fail();
      }
      r.minimum = item["minimum"];
    }
    if (r.hasMaximum) {
      if (!number(item["maximum"])) {
        return fail();
      }
      r.maximum = item["maximum"];
    }
    for (const char* key : {"below_percent", "above_percent"}) {
      if (!item[key].isNull() && (!item[key].is<unsigned>() || item[key].as<unsigned>() > 100)) {
        return fail();
      }
    }
    r.belowPercent = item["below_percent"].isNull() ? -1 : item["below_percent"].as<int>();
    r.abovePercent = item["above_percent"].isNull() ? -1 : item["above_percent"].as<int>();
    if ((!r.hasMinimum && r.belowPercent >= 0) || (!r.hasMaximum && r.abovePercent >= 0)) {
      return fail();
    }
  }
  return valid(next, analog, pulse, maxRuntime) || fail();
}
void append(JsonObject row, const Config& c) {
  auto a = row["automation"].to<JsonObject>();
  a["fallback_percent"] = c.fallbackPercent;
  auto rules = a["rules"].to<JsonArray>();
  for (unsigned n = 0; n < c.count; ++n) {
    const auto& r = c.rules[n];
    auto v = rules.add<JsonObject>();
    v["source"] = r.source;
    v["signal"] = signalRegistry_name(static_cast<SignalId>(r.signal));
    v["direction"] = r.raise ? "raise" : "lower";
    v["target"] = r.target;
    v["band"] = r.band;
    v["hysteresis"] = r.hysteresis;
    v["delay_s"] = r.delaySeconds;
    if (r.hasMinimum) {
      v["minimum"] = r.minimum;
    }
    if (r.hasMaximum) {
      v["maximum"] = r.maximum;
    }
    if (r.belowPercent >= 0) {
      v["below_percent"] = r.belowPercent;
    }
    if (r.abovePercent >= 0) {
      v["above_percent"] = r.abovePercent;
    }
  }
}
void appendDecision(JsonObject row, const Decision& d, const Config& config, const char* block) {
  auto r = row["automation_status"].to<JsonObject>();
  r["reason"] = d.reason;
  r["demand_percent"] = d.percent;
  r["winning_rule"] = d.winner;
  r["warning_mask"] = d.warnings;
  r["execution_block"] = block ? block : (!d.evaluated ? "WAIT_EVALUATION" : "NONE");
  const uint32_t elapsed = uint32_t(millis() - d.evaluatedAt);
  if (d.evaluated) {
    r["evaluated_age_ms"] = elapsed;
  } else {
    r["evaluated_age_ms"] = nullptr;
  }
  auto rules = r["rules"].to<JsonArray>();
  for (unsigned n = 0; n < config.count; ++n) {
    const auto& rule = config.rules[n];
    const auto& sample = d.rules[n];
    const bool live = block || !d.evaluated;
    auto input = live ? read(rule.signal, rule.source) : sample.input;
    if (!live && input.ageMs != UINT32_MAX) {
      input.ageMs =
          uint64_t(input.ageMs) + elapsed >= UINT32_MAX ? UINT32_MAX - 1 : input.ageMs + elapsed;
    }
    const bool valid =
        input.valid && isfinite(input.value) && input.ageMs < founderAutomation::InputMaxAgeMs;
    const char* inputState = valid                                             ? "OK"
                             : input.ageMs == UINT32_MAX                       ? "MISSING"
                             : input.ageMs >= founderAutomation::InputMaxAgeMs ? "STALE"
                                                                               : "INVALID";
    auto item = rules.add<JsonObject>();
    item["priority"] = n + 1;
    item["source"] = rule.source;
    item["signal"] = signalRegistry_name(static_cast<SignalId>(rule.signal));
    item["unit"] = signalRegistry_unit(static_cast<SignalId>(rule.signal));
    item["input_state"] = inputState;
    item["valid"] = valid;
    if (input.ageMs == UINT32_MAX) {
      item["age_ms"] = nullptr;
    } else {
      item["age_ms"] = input.ageMs;
    }
    if (valid) {
      item["value"] = input.value;
    } else {
      item["value"] = nullptr;
    }
    const char* observed = valid && rule.hasMinimum && input.value < rule.minimum ? "BELOW_MINIMUM"
                           : valid && rule.hasMaximum && input.value > rule.maximum
                               ? "ABOVE_MAXIMUM"
                               : "NONE";
    item["observed_limit"] = observed;
    item["active_limit"] = !live && sample.zone == -2  ? "BELOW_MINIMUM"
                           : !live && sample.zone == 2 ? "ABOVE_MAXIMUM"
                                                       : "NONE";
    const bool applicable = !live && !d.missingMask && valid;
    if (applicable) {
      item["normal_demand_percent"] = sample.normalPercent;
      item["requested_percent"] = sample.requestedPercent;
    } else {
      item["normal_demand_percent"] = nullptr;
      item["requested_percent"] = nullptr;
    }
    item["selected"] = applicable && d.winner == int(n);
    if (applicable && sample.action >= 0) {
      item["limit_action_percent"] = sample.action;
    } else {
      item["limit_action_percent"] = nullptr;
    }
    item["pending_ms"] =
        applicable ? (sample.pendingMs > elapsed ? sample.pendingMs - elapsed : 0) : 0;
    item["decision"] = block                        ? block
                       : !d.evaluated               ? "WAIT_EVALUATION"
                       : !valid                     ? inputState
                       : d.missingMask              ? "FALLBACK"
                       : sample.pendingMs > elapsed ? "DELAY"
                       : d.winner == int(n)         ? "SELECTED"
                       : sample.action >= 0 || !strcmp(d.reason, "BELOW_MINIMUM") ||
                               !strcmp(d.reason, "ABOVE_MAXIMUM")
                           ? "LOWER_PRIORITY"
                       : sample.normalPercent && sample.normalPercent == d.percent ? "SAME_DEMAND"
                       : sample.normalPercent ? "LOWER_DEMAND"
                                              : "TARGET_REACHED";
  }
}
Reading read(unsigned id, const char* source) {
  return founderDevices::readSignal(static_cast<SignalId>(id), source);
}
}  // namespace founderAutomation
