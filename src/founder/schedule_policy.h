#pragma once
#include <stdint.h>
namespace founderSchedule {
constexpr unsigned MaxSlots = 8;
struct Slot {
  uint16_t start = 0, end = 0, runtimeMs = 0;
  uint8_t percent = 100;
};
struct Interval {
  bool enabled = false;
  uint16_t anchor = 0, everyMinutes = 180, durationMinutes = 10, runtimeMs = 1000;
  uint8_t percent = 100;
};
inline bool same(const Interval& a, const Interval& b) {
  return a.enabled == b.enabled && a.anchor == b.anchor && a.everyMinutes == b.everyMinutes &&
         a.durationMinutes == b.durationMinutes && a.runtimeMs == b.runtimeMs &&
         a.percent == b.percent;
}
struct Plan {
  Interval interval;
  bool enabled = false;
  uint8_t count = 0, catchUpMinutes = 0;
  uint32_t sinceToken = UINT32_MAX;
  Slot slots[MaxSlots];
};
inline bool contains(const Slot& s, unsigned minute) {
  return s.start < s.end ? minute >= s.start && minute < s.end
                         : minute >= s.start || minute < s.end;
}
inline bool valid(const Plan& p, bool pulse, unsigned maxRuntime) {
  if (p.interval.enabled) {
    const auto& i = p.interval;
    return !p.count && i.anchor < 1440 && i.everyMinutes >= 1 && i.everyMinutes <= 1440 &&
           p.catchUpMinutes <= 60 &&
           (pulse ? i.runtimeMs >= 1 && i.runtimeMs <= maxRuntime
                  : !p.catchUpMinutes && i.durationMinutes >= 1 &&
                        i.durationMinutes <= i.everyMinutes && i.percent >= 1 && i.percent <= 100);
  }
  if (p.count > MaxSlots || (p.enabled && !p.count) || p.catchUpMinutes > 60 ||
      (!pulse && p.catchUpMinutes)) {
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
  if (p.interval.enabled) {
    const auto& i = p.interval;
    const unsigned last = minute >= i.anchor
                              ? i.anchor + (minute - i.anchor) / i.everyMinutes * i.everyMinutes
                              : i.anchor + (1439 - i.anchor) / i.everyMinutes * i.everyMinutes;
    const unsigned age = (minute + 1440 - last) % 1440;
    return age < i.durationMinutes ? i.percent : 0;
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
inline unsigned previousDate(unsigned date) {
  if (date % 1000) {
    return date - 1;
  }
  const unsigned year = date / 1000 - 1;
  const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
  return year * 1000 + (leap ? 365 : 364);
}
struct Occurrence {
  uint32_t token = 0;
  unsigned slot = 0, ageMinutes = 0;
};
inline Occurrence latest(const Plan& plan, unsigned date, unsigned minute) {
  Occurrence result;
  const unsigned count = plan.interval.enabled
                             ? 1 + (1439 - plan.interval.anchor) / plan.interval.everyMinutes
                             : plan.count;
  for (unsigned n = 0; n < count; ++n) {
    const unsigned at = plan.interval.enabled
                            ? plan.interval.anchor + n * plan.interval.everyMinutes
                            : plan.slots[n].start;
    const bool yesterday = at > minute;
    const unsigned stamp = token(yesterday ? previousDate(date) : date, at);
    if (stamp > result.token) {
      result.token = stamp;
      result.slot = n;
      result.ageMinutes = yesterday ? minute + 1440 - at : minute - at;
    }
  }
  return result;
}
}  // namespace founderSchedule
