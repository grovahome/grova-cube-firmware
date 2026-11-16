#pragma once
#include <stdint.h>
#include <string.h>

#include "controller/automation_policy.h"
namespace founderEvents {
constexpr unsigned Capacity = 64, Devices = 7;
enum class Type : uint8_t {
  Boot,
  ConfigurationSaved,
  Paused,
  Released,
  InputLost,
  InputRecovered,
  Minimum,
  Maximum,
  LimitCleared,
  FallbackRequired,
  FallbackActive,
  FallbackEnded
};
struct Event {
  uint64_t sequence = 0;
  uint32_t uptimeMs = 0, epoch = 0, revision = 0;
  Type type = Type::Boot;
  int8_t device = -1, rule = -1;
  uint8_t signal = 0, percent = 0, inputState = 0;
  char name[40] = {}, source[40] = {};
  float value = 0, limit = 0;
  bool hasValue = false, hasLimit = false;
};
struct RuleTrack {
  bool known = false, available = false;
  int8_t zone = 0, pending = 0;
  uint32_t since = 0;
};
struct DeviceTrack {
  RuleTrack rules[founderAutomation::MaxRules];
  uint8_t fallback = 0;
};
class Recorder {
 public:
  Event entries[Capacity];
  unsigned count = 0, next = 0;
  uint64_t total = 0;
  DeviceTrack tracks[Devices];
  bool restKnown = false, rest = true;
  void add(Type type, uint32_t now, uint32_t epoch, int device = -1, const char* name = "",
           int rule = -1, const founderAutomation::Rule* config = nullptr,
           const founderAutomation::Reading* input = nullptr, unsigned percent = 0,
           unsigned inputState = 0, uint32_t revision = 0) {
    auto& e = entries[next];
    e = Event{};
    e.sequence = ++total;
    e.type = type;
    e.uptimeMs = now;
    e.epoch = epoch;
    e.device = device;
    e.rule = rule;
    e.percent = percent;
    e.inputState = inputState;
    e.revision = revision;
    strncpy(e.name, name, sizeof(e.name) - 1);
    if (config) {
      strncpy(e.source, config->source, sizeof(e.source) - 1);
      e.signal = config->signal;
    }
    if (config && (type == Type::Minimum || type == Type::Maximum || type == Type::LimitCleared)) {
      e.hasLimit = true;
      e.limit = type == Type::Minimum || (type == Type::LimitCleared && inputState == 1)
                    ? config->minimum
                    : config->maximum;
    }
    if (input && input->valid && isfinite(input->value) &&
        input->ageMs < founderAutomation::InputMaxAgeMs) {
      e.value = input->value;
      e.hasValue = true;
    }
    next = (next + 1) % Capacity;
    if (count < Capacity) {
      ++count;
    }
  }
  const Event& newest(unsigned offset) const {
    return entries[(next + Capacity - 1 - offset) % Capacity];
  }
  void begin(uint32_t now, uint32_t epoch) {
    count = next = 0;
    total = 0;
    restKnown = false;
    for (auto& t : tracks) {
      t = DeviceTrack{};
    }
    add(Type::Boot, now, epoch);
  }
  void configuration(uint32_t revision, uint32_t now, uint32_t epoch) {
    for (auto& t : tracks) {
      t = DeviceTrack{};
    }
    add(Type::ConfigurationSaved, now, epoch, -1, "", -1, nullptr, nullptr, 0, 0, revision);
  }
  void operation(bool paused, uint32_t now, uint32_t epoch) {
    if (!restKnown || rest != paused) {
      restKnown = true;
      rest = paused;
      add(paused ? Type::Paused : Type::Released, now, epoch);
    }
  }
  template <class Reader>
  void observe(unsigned device, const char* name, const founderAutomation::Config& c, bool enabled,
               bool canRun, uint32_t now, uint32_t epoch, Reader read) {
    if (device >= Devices) {
      return;
    }
    auto& t = tracks[device];
    if (!enabled || !c.count) {
      t = DeviceTrack{};
      return;
    }
    bool missing = false;
    for (unsigned n = 0; n < c.count; ++n) {
      const auto& r = c.rules[n];
      auto& s = t.rules[n];
      const auto input = read(r.signal, r.source);
      const bool available =
          input.valid && isfinite(input.value) && input.ageMs < founderAutomation::InputMaxAgeMs;
      if (!available) {
        missing = true;
        if (!s.known || s.available) {
          add(Type::InputLost, now, epoch, device, name, n, &r, nullptr, 0,
              input.ageMs == UINT32_MAX                         ? 1
              : input.ageMs >= founderAutomation::InputMaxAgeMs ? 2
                                                                : 3);
        }
        s.known = true;
        s.available = false;
        s.zone = s.pending = 0;
        s.since = now;
        continue;
      }
      if (s.known && !s.available) {
        add(Type::InputRecovered, now, epoch, device, name, n, &r, &input);
      }
      s.known = s.available = true;
      int zone = r.hasMinimum && (input.value < r.minimum ||
                                  (s.zone == -1 && input.value < r.minimum + r.hysteresis))
                     ? -1
                 : r.hasMaximum && (input.value > r.maximum ||
                                    (s.zone == 1 && input.value > r.maximum - r.hysteresis))
                     ? 1
                     : 0;
      if (zone != s.pending) {
        s.pending = zone;
        s.since = now;
      }
      if (s.pending != s.zone && uint32_t(now - s.since) >= uint32_t(r.delaySeconds) * 1000) {
        if (s.zone) {
          add(Type::LimitCleared, now, epoch, device, name, n, &r, &input, 0, s.zone < 0 ? 1 : 2);
        }
        s.zone = s.pending;
        if (s.zone) {
          add(s.zone < 0 ? Type::Minimum : Type::Maximum, now, epoch, device, name, n, &r, &input);
        }
      }
    }
    const uint8_t fallback = missing ? (canRun ? 2 : 1) : 0;
    if (fallback != t.fallback) {
      if (fallback) {
        add(fallback == 2 ? Type::FallbackActive : Type::FallbackRequired, now, epoch, device, name,
            -1, nullptr, nullptr, c.fallbackPercent);
      } else if (t.fallback) {
        add(Type::FallbackEnded, now, epoch, device, name);
      }
      t.fallback = fallback;
    }
  }
};
}  // namespace founderEvents
