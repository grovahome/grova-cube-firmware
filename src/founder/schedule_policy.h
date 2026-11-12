#pragma once
#include <stdint.h>
namespace founderSchedule {
constexpr unsigned MaxSlots = 8;
struct Slot {
  uint16_t start = 0, end = 0, runtimeMs = 0;
  uint8_t percent = 100;
};
struct Plan {
  bool enabled = false;
  uint8_t count = 0;
  Slot slots[MaxSlots];
};
inline bool contains(const Slot& s, unsigned minute) {
  return s.start < s.end ? minute >= s.start && minute < s.end
                         : minute >= s.start || minute < s.end;
}
inline bool valid(const Plan& p, bool pulse, unsigned maxRuntime) {
  if (p.count > MaxSlots || (p.enabled && !p.count)) {
    return false;
  }
  for (unsigned i = 0; i < p.count; ++i) {
    const auto& s = p.slots[i];
    if (s.start >= 1440) {
      return false;
    }
    if (pulse) {
      if (!s.runtimeMs || s.runtimeMs > maxRuntime || s.end) {
        return false;
      }
    } else if (s.end > 1440 || s.start == s.end || !s.percent || s.percent > 100 || s.runtimeMs) {
      return false;
    }
    for (unsigned j = 0; j < i; ++j) {
      if (pulse && s.start == p.slots[j].start) {
        return false;
      }
      if (!pulse) {
        for (unsigned m = 0; m < 1440; ++m) {
          if (contains(s, m) && contains(p.slots[j], m)) {
            return false;
          }
        }
      }
    }
  }
  return true;
}
inline unsigned percent(const Plan& p, unsigned minute) {
  if (!p.enabled || minute >= 1440) {
    return 0;
  }
  for (unsigned i = 0; i < p.count; ++i) {
    if (contains(p.slots[i], minute)) {
      return p.slots[i].percent;
    }
  }
  return 0;
}
inline uint32_t token(unsigned dateKey, unsigned minute) {
  return dateKey * 1440U + minute;
}
}  // namespace founderSchedule
