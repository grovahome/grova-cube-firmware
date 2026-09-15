#include <Arduino.h>
#include <driver/timer.h>
#include <esp_timer.h>
#include <soc/gpio_reg.h>
#include "config.h"

// PCB v1 reserves timer group 1 / timer 0 for pump safety. No Arduino loop,
// network task, NVS operation, or esp_timer task dispatch is involved in cutoff.
// A periodic interrupt checks an absolute monotonic deadline: delayed interrupts
// do not extend the run by counting missed ticks. Resolution is 1 ms plus ISR
// latency; this is not protection against a CPU/interrupt or electrical failure.
static constexpr timer_group_t PUMP_TIMER_GROUP = TIMER_GROUP_1;
static constexpr timer_idx_t PUMP_TIMER_INDEX = TIMER_0;
static constexpr uint64_t PUMP_TIMER_TICK_US = 1000;
static portMUX_TYPE pumpLock = portMUX_INITIALIZER_UNLOCKED;
static bool initialized = false;
static bool cutoffReady = false;
static bool inhibited = false;
static bool running = false;
static int64_t deadlineUs = 0;

static_assert(PIN_PUMP >= 0 && PIN_PUMP <= 33, "Pump requires an ESP32 output GPIO");

static void IRAM_ATTR writePumpLow() {
  // Direct register access stays available when the flash cache is disabled.
  if (PIN_PUMP < 32) REG_WRITE(GPIO_OUT_W1TC_REG, 1UL << (PIN_PUMP % 32));
  else REG_WRITE(GPIO_OUT1_W1TC_REG, 1UL << (PIN_PUMP % 32));
}

static bool IRAM_ATTR cutoffInterrupt(void*) {
  portENTER_CRITICAL_ISR(&pumpLock);
  if (running && esp_timer_get_time() >= deadlineUs) {
    writePumpLow();
    running = false;
    deadlineUs = 0;
  }
  portEXIT_CRITICAL_ISR(&pumpLock);
  return false;
}

void pump_begin() {
  if (initialized) return;
  initialized = true;
  digitalWrite(PIN_PUMP, LOW);
  pinMode(PIN_PUMP, OUTPUT);
  writePumpLow();

  timer_config_t config = {};
  config.alarm_en = TIMER_ALARM_EN;
  config.counter_en = TIMER_PAUSE;
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_dir = TIMER_COUNT_UP;
  config.auto_reload = TIMER_AUTORELOAD_EN;
  config.divider = 80; // ESP32 APB 80 MHz -> 1 microsecond per tick.

  if (timer_init(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX, &config) != ESP_OK) {
    Serial.println("Pump cutoff unavailable; pump disabled");
    return;
  }
  bool callbackAdded = false;
  bool ok = timer_set_counter_value(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX, 0) == ESP_OK &&
    timer_set_alarm_value(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX, PUMP_TIMER_TICK_US) == ESP_OK;
  if (ok) {
    callbackAdded = timer_isr_callback_add(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX,
      cutoffInterrupt, nullptr, ESP_INTR_FLAG_IRAM) == ESP_OK;
    ok = callbackAdded;
  }
  if (ok) ok = timer_enable_intr(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX) == ESP_OK;
  if (ok) ok = timer_start(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX) == ESP_OK;
  if (!ok) {
    if (callbackAdded) timer_isr_callback_remove(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX);
    timer_deinit(PUMP_TIMER_GROUP, PUMP_TIMER_INDEX);
    Serial.println("Pump cutoff unavailable; pump disabled");
    return;
  }
  cutoffReady = true;
}

bool pump_startTimed(unsigned long runtimeMs) {
  if (runtimeMs == 0 || runtimeMs > static_cast<unsigned long>(PUMP_RUNTIME_MAX_SECONDS) * 1000UL) return false;
  portENTER_CRITICAL(&pumpLock);
  if (!cutoffReady || inhibited || running) {
    portEXIT_CRITICAL(&pumpLock);
    return false;
  }
  deadlineUs = esp_timer_get_time() + static_cast<int64_t>(runtimeMs) * 1000;
  running = true;
  // Arm the deadline before energizing; the ISR shares this short critical section.
  if (PIN_PUMP < 32) REG_WRITE(GPIO_OUT_W1TS_REG, 1UL << (PIN_PUMP % 32));
  else REG_WRITE(GPIO_OUT1_W1TS_REG, 1UL << (PIN_PUMP % 32));
  portEXIT_CRITICAL(&pumpLock);
  return true;
}

bool pump_isRunning() {
  portENTER_CRITICAL(&pumpLock);
  bool result = running;
  portEXIT_CRITICAL(&pumpLock);
  return result;
}

void pump_inhibit() {
  portENTER_CRITICAL(&pumpLock);
  inhibited = true;
  writePumpLow();
  running = false;
  deadlineUs = 0;
  portEXIT_CRITICAL(&pumpLock);
}

void pump_off() {
  portENTER_CRITICAL(&pumpLock);
  writePumpLow();
  running = false;
  deadlineUs = 0;
  portEXIT_CRITICAL(&pumpLock);
}
