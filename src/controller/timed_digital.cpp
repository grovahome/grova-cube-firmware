#include "controller/timed_digital.h"

#include <Arduino.h>
#include <driver/timer.h>
#include <esp_timer.h>
#include <soc/gpio_reg.h>

#include "boards/grova_core_s3.h"
namespace founderDigital {
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static DRAM_ATTR uint32_t masks[3] = {1UL << founder::OUT12_1, 1UL << founder::OUT5_1,
                                      1UL << founder::OUT12_2};
struct State {
  bool on = false;
  int64_t deadline = 0;
};
static State channels[3];
static bool initialized = false, available = false;
static bool IRAM_ATTR cutoff(void*) {
  portENTER_CRITICAL_ISR(&lock);
  const int64_t now = esp_timer_get_time();
  for (unsigned i = 0; i < 3; ++i) {
    if (channels[i].on && channels[i].deadline && now >= channels[i].deadline) {
      REG_WRITE(GPIO_OUT_W1TC_REG, masks[i]);
      channels[i].on = false;
      channels[i].deadline = 0;
    }
  }
  portEXIT_CRITICAL_ISR(&lock);
  return false;
}
bool begin() {
  if (initialized) {
    return available;
  }
  initialized = true;
  timer_config_t config = {};
  config.alarm_en = TIMER_ALARM_EN;
  config.counter_en = TIMER_PAUSE;
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_dir = TIMER_COUNT_UP;
  config.auto_reload = TIMER_AUTORELOAD_EN;
  config.divider = 80;
  if (timer_init(TIMER_GROUP_1, TIMER_0, &config) != ESP_OK) {
    return false;
  }
  bool added = false;
  bool ok = timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0) == ESP_OK &&
            timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, 1000) == ESP_OK;
  if (ok) {
    added = timer_isr_callback_add(TIMER_GROUP_1, TIMER_0, cutoff, nullptr, ESP_INTR_FLAG_IRAM) ==
            ESP_OK;
    ok = added;
  }
  if (ok) {
    ok = timer_enable_intr(TIMER_GROUP_1, TIMER_0) == ESP_OK;
  }
  if (ok) {
    ok = timer_start(TIMER_GROUP_1, TIMER_0) == ESP_OK;
  }
  if (!ok) {
    if (added) {
      timer_isr_callback_remove(TIMER_GROUP_1, TIMER_0);
    }
    timer_deinit(TIMER_GROUP_1, TIMER_0);
  }
  available = ok;
  return ok;
}
bool ready() {
  return available;
}
void set(unsigned i, bool on) {
  if (i >= 3) {
    return;
  }
  portENTER_CRITICAL(&lock);
  channels[i].deadline = 0;
  channels[i].on = on && available;
  REG_WRITE(channels[i].on ? GPIO_OUT_W1TS_REG : GPIO_OUT_W1TC_REG, masks[i]);
  portEXIT_CRITICAL(&lock);
}
bool pulse(unsigned i, unsigned durationMs) {
  if (i >= 3 || durationMs == 0 || durationMs > MaxRuntimeMs) {
    return false;
  }
  portENTER_CRITICAL(&lock);
  if (!available || channels[i].on) {
    portEXIT_CRITICAL(&lock);
    return false;
  }
  channels[i].deadline = esp_timer_get_time() + int64_t(durationMs) * 1000;
  channels[i].on = true;
  REG_WRITE(GPIO_OUT_W1TS_REG, masks[i]);
  portEXIT_CRITICAL(&lock);
  return true;
}
bool on(unsigned i) {
  if (i >= 3) {
    return false;
  }
  portENTER_CRITICAL(&lock);
  bool value = channels[i].on;
  portEXIT_CRITICAL(&lock);
  return value;
}
unsigned remainingMs(unsigned i) {
  if (i >= 3) {
    return 0;
  }
  portENTER_CRITICAL(&lock);
  int64_t remaining = channels[i].deadline ? channels[i].deadline - esp_timer_get_time() : 0;
  portEXIT_CRITICAL(&lock);
  return remaining > 0 ? unsigned((remaining + 999) / 1000) : 0;
}
}  // namespace founderDigital
