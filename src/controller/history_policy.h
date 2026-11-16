#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace founderHistory {
constexpr unsigned Capacity = 120, MaxSeries = 64;
constexpr uint32_t IntervalMs = 30000;
struct Stamp {
  uint32_t uptime = 0, epoch = 0;
};
struct Series {
  char source[40] = {};
  uint8_t metric = 0;
  bool used = false;
  float values[Capacity];
};
class Recorder {
 public:
  Stamp stamps[Capacity];
  Series series[MaxSeries];
  unsigned count = 0, next = 0, current = 0, omitted = 0;
  bool sampled = false;
  uint32_t last = 0;
  void begin() {
    count = next = current = omitted = 0;
    sampled = false;
    last = 0;
    for (auto& s : series) {
      s.used = false;
      for (auto& v : s.values) {
        v = NAN;
      }
    }
  }
  bool start(uint32_t now, uint32_t epoch) {
    if (sampled && uint32_t(now - last) < IntervalMs) {
      return false;
    }
    sampled = true;
    last = now;
    current = next;
    stamps[current].uptime = now;
    stamps[current].epoch = epoch;
    next = (next + 1) % Capacity;
    if (count < Capacity) {
      ++count;
    }
    omitted = 0;
    for (auto& s : series) {
      s.values[current] = NAN;
      if (s.used) {
        bool valid = false;
        for (float v : s.values) {
          valid = valid || isfinite(v);
        }
        if (!valid) {
          s.used = false;
        }
      }
    }
    return true;
  }
  bool put(const char* source, unsigned metric, float value) {
    Series* target = nullptr;
    for (auto& s : series) {
      if (s.used && s.metric == metric && !strcmp(s.source, source)) {
        target = &s;
        break;
      }
    }
    if (!target) {
      for (auto& s : series) {
        if (!s.used) {
          target = &s;
          strncpy(s.source, source, sizeof(s.source) - 1);
          s.source[sizeof(s.source) - 1] = 0;
          s.metric = metric;
          s.used = true;
          for (auto& v : s.values) {
            v = NAN;
          }
          break;
        }
      }
    }
    if (!target) {
      ++omitted;
      return false;
    }
    target->values[current] = isfinite(value) ? value : NAN;
    return true;
  }
  unsigned position(unsigned chronological) const {
    return (next + Capacity - count + chronological) % Capacity;
  }
};
}  // namespace founderHistory
