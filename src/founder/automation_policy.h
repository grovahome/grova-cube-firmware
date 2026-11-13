#pragma once
#include <math.h>
#include <stdint.h>

#include <initializer_list>
namespace founderAutomation {
constexpr unsigned MaxRules = 6;
struct Rule {
  char source[40] = {};
  uint8_t signal = 0;
  bool raise = false, hasMinimum = false, hasMaximum = false;
  float target = 0, minimum = 0, maximum = 0, band = 2, hysteresis = 0.5f;
  int16_t belowPercent = -1, abovePercent = -1;
  uint16_t delaySeconds = 5;
};
struct Config {
  uint8_t count = 0, fallbackPercent = 0;
  uint16_t pulseMs = 1000;
  uint32_t cooldownSeconds = 3600;
  Rule rules[MaxRules];
};
struct Reading {
  Reading(float sample = 0, bool usable = false, uint32_t age = UINT32_MAX)
      : value(sample), valid(usable), ageMs(age) {}
  float value = 0;
  bool valid = false;
  uint32_t ageMs = UINT32_MAX;
};
struct RuleState {
  int8_t zone = 0, pending = 0;
  uint32_t since = 0;
};
struct State {
  RuleState rules[MaxRules];
};
struct RuleDiagnostic {
  Reading input;
  unsigned normalPercent = 0, requestedPercent = 0;
  int16_t action = -1;
  int8_t zone = 0, pending = 0;
  uint32_t pendingMs = 0;
};
struct Decision {
  RuleDiagnostic rules[MaxRules];
  uint32_t evaluatedAt = 0;
  uint8_t missingMask = 0;
  bool evaluated = false;
  unsigned percent = 0;
  int winner = -1;
  uint16_t warnings = 0;
  const char* reason = "NOT_CONFIGURED";
};
inline bool valid(const Config& c, bool analog, bool pulse, unsigned maxRuntime) {
  if (c.count > MaxRules || c.fallbackPercent > 100 ||
      (!analog && c.fallbackPercent != 0 && c.fallbackPercent != 100) ||
      (pulse && (c.fallbackPercent != 0 || !c.pulseMs || c.pulseMs > maxRuntime ||
                 c.cooldownSeconds < 60 || c.cooldownSeconds > 86400))) {
    return false;
  }
  for (unsigned n = 0; n < c.count; ++n) {
    const auto& r = c.rules[n];
    if (!*r.source || r.signal >= 6 || !isfinite(r.target) || !isfinite(r.band) || r.band <= 0 ||
        !isfinite(r.hysteresis) || r.hysteresis < 0 || r.hysteresis > r.band ||
        r.delaySeconds > 300 || (r.hasMinimum && (!isfinite(r.minimum) || r.minimum >= r.target)) ||
        (r.hasMaximum && (!isfinite(r.maximum) || r.maximum <= r.target)) ||
        (r.hasMinimum && r.hasMaximum && r.hysteresis >= r.maximum - r.minimum)) {
      return false;
    }
    for (int action : {int(r.belowPercent), int(r.abovePercent)}) {
      if (action < -1 || action > 100 ||
          (!analog && action != -1 && action != 0 && action != 100)) {
        return false;
      }
    }
  }
  return true;
}
// Reader resolves the exact persisted device source, never a global best-source alias.
template <class Reader>
Decision evaluate(const Config& c, State& state, uint32_t now, bool analog, Reader read) {
  Decision result;
  result.evaluated = true;
  result.evaluatedAt = now;
  if (!c.count) {
    return result;
  }
  Reading values[MaxRules];
  for (unsigned n = 0; n < c.count; ++n) {
    values[n] = read(c.rules[n].signal, c.rules[n].source);
    result.rules[n].input = values[n];
    if (!values[n].valid || !isfinite(values[n].value) || values[n].ageMs >= 12000) {
      result.missingMask |= uint8_t(1U << n);
      if (result.winner < 0) {
        result.winner = n;
      }
    }
  }
  if (result.missingMask) {
    state = State{};
    result.percent = c.fallbackPercent;
    result.reason = "SENSOR_MISSING";
    return result;
  }
  result.reason = "TARGET_REACHED";
  int overrideWinner = -1;
  for (unsigned n = 0; n < c.count; ++n) {
    const auto& r = c.rules[n];
    auto& s = state.rules[n];
    const float v = values[n].value;
    const float deviation = r.raise ? r.target - v : v - r.target;
    int zone = 0;
    if (r.hasMinimum && (v < r.minimum || (s.zone == -2 && v < r.minimum + r.hysteresis))) {
      zone = -2;
    } else if (r.hasMaximum && (v > r.maximum || (s.zone == 2 && v > r.maximum - r.hysteresis))) {
      zone = 2;
    } else if (deviation > (s.zone == 1 ? 0 : r.hysteresis)) {
      zone = 1;
    }
    if (zone != s.pending) {
      s.pending = zone;
      s.since = now;
    }
    if (uint32_t(now - s.since) >= uint32_t(r.delaySeconds) * 1000) {
      s.zone = s.pending;
    }
    const bool warning = s.zone == -2 || s.zone == 2;
    if (warning) {
      result.warnings |= uint16_t(1U << (n * 2 + (s.zone == 2 ? 1 : 0)));
    }
    const int special = s.zone == -2 ? r.belowPercent : s.zone == 2 ? r.abovePercent : -1;
    unsigned demand = 0;
    if (s.zone && deviation > 0) {
      demand = analog ? unsigned(fminf(100, fmaxf(1, ceilf(deviation / r.band * 100)))) : 100;
    }
    auto& diagnostic = result.rules[n];
    diagnostic.normalPercent = demand;
    diagnostic.requestedPercent = warning && special >= 0 ? unsigned(special) : demand;
    diagnostic.action = warning ? special : -1;
    diagnostic.zone = s.zone;
    diagnostic.pending = s.pending;
    const uint32_t wait = uint32_t(r.delaySeconds) * 1000, elapsed = uint32_t(now - s.since);
    diagnostic.pendingMs = s.zone != s.pending && elapsed < wait ? wait - elapsed : 0;
    if (warning && special >= 0 && overrideWinner < 0) {
      overrideWinner = n;
      result.percent = special;
      result.winner = n;
      result.reason = s.zone == -2 ? "BELOW_MINIMUM" : "ABOVE_MAXIMUM";
    }
    if (overrideWinner >= 0) {
      continue;
    }
    if (demand > result.percent) {
      result.percent = demand;
      result.winner = n;
      result.reason = "TARGET_CONTROL";
    }
  }
  return result;
}
}  // namespace founderAutomation
