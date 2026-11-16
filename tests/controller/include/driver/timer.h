#pragma once
#include <stdint.h>
#define ESP_OK 0
#define ESP_INTR_FLAG_IRAM 1
#define TIMER_GROUP_1 1
#define TIMER_0 0
#define TIMER_ALARM_EN 1
#define TIMER_PAUSE 0
#define TIMER_INTR_LEVEL 1
#define TIMER_COUNT_UP 1
#define TIMER_AUTORELOAD_EN 1
struct timer_config_t {
  int alarm_en, counter_en, intr_type, counter_dir, auto_reload, divider;
};
namespace testTimer {
inline bool fail = false;
inline bool (*callback)(void*) = nullptr;
inline void fire() {
  if (callback) {
    callback(nullptr);
  }
}
}  // namespace testTimer
inline int timer_init(int, int, const timer_config_t*) {
  return testTimer::fail ? -1 : 0;
}
inline int timer_set_counter_value(int, int, uint64_t) {
  return 0;
}
inline int timer_set_alarm_value(int, int, uint64_t) {
  return 0;
}
inline int timer_isr_callback_add(int, int, bool (*cb)(void*), void*, int) {
  testTimer::callback = cb;
  return 0;
}
inline int timer_isr_callback_remove(int, int) {
  testTimer::callback = nullptr;
  return 0;
}
inline int timer_enable_intr(int, int) {
  return 0;
}
inline int timer_start(int, int) {
  return 0;
}
inline int timer_deinit(int, int) {
  return 0;
}
