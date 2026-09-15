// Bench only: disconnect the pump/load before uploading this test firmware.
#include <Arduino.h>
#include <driver/timer.h>
#include <unity.h>

// Compile the real driver without the application's network/scheduler setup.
static bool failTimerInit = false;
static bool failTimerStart = false;
static esp_err_t checkedTimerInit(timer_group_t group, timer_idx_t index, const timer_config_t* config) {
  return failTimerInit ? ESP_FAIL : timer_init(group, index, config);
}
static esp_err_t checkedTimerStart(timer_group_t group, timer_idx_t index) {
  return failTimerStart ? ESP_FAIL : timer_start(group, index);
}
#define timer_init checkedTimerInit
#define timer_start checkedTimerStart
#include "../../src/modules/pump.cpp"
#undef timer_init
#undef timer_start

void setUp() { pump_off(); }
void tearDown() { pump_off(); }

static bool outputHigh() {
  if (PIN_PUMP < 32) return (REG_READ(GPIO_OUT_REG) & (1UL << (PIN_PUMP % 32))) != 0;
  return (REG_READ(GPIO_OUT1_REG) & (1UL << (PIN_PUMP % 32))) != 0;
}

void rejects_start_before_initialization() {
  TEST_ASSERT_FALSE(pump_startTimed(50));
  TEST_ASSERT_FALSE(pump_isRunning());
}

void initialization_failure_is_fail_closed() {
  failTimerInit = true;
  pump_begin();
  TEST_ASSERT_FALSE(pump_startTimed(50));
  TEST_ASSERT_FALSE(outputHigh());
  failTimerInit = false;
  pump_begin();
  TEST_ASSERT_FALSE(pump_startTimed(50));
  initialized = false; // Test-only cold boot; failed init allocated no timer.
}

void timer_start_failure_is_fail_closed() {
  failTimerStart = true;
  pump_begin();
  TEST_ASSERT_FALSE(pump_startTimed(50));
  TEST_ASSERT_FALSE(outputHigh());
  failTimerStart = false;
  initialized = false; // Failure path must have released timer and callback.
}

void validates_duration_and_initializes_once() {
  pump_begin();
  TEST_ASSERT_TRUE(cutoffReady);
  TEST_ASSERT_FALSE(pump_startTimed(0));
  TEST_ASSERT_FALSE(pump_startTimed(10001));
  TEST_ASSERT_FALSE(pump_startTimed(0xffffffffUL));
  TEST_ASSERT_FALSE(outputHigh());
  TEST_ASSERT_TRUE(pump_startTimed(50));
  pump_begin();
  TEST_ASSERT_TRUE(pump_isRunning());
  TEST_ASSERT_TRUE(outputHigh());
}

void cuts_off_without_scheduler_or_cooperative_yield() {
  TEST_ASSERT_TRUE(pump_startTimed(50));
  const int64_t blockedUntil = esp_timer_get_time() + 200000;
  while (esp_timer_get_time() < blockedUntil) { asm volatile("nop"); }
  TEST_ASSERT_FALSE(outputHigh());
  TEST_ASSERT_FALSE(pump_isRunning());
}

void busy_start_cannot_extend_deadline() {
  TEST_ASSERT_TRUE(pump_startTimed(50));
  delay(20);
  TEST_ASSERT_FALSE(pump_startTimed(1000));
  delay(50);
  TEST_ASSERT_FALSE(outputHigh());
  TEST_ASSERT_FALSE(pump_isRunning());
}

void old_deadline_cannot_stop_restarted_run() {
  TEST_ASSERT_TRUE(pump_startTimed(50));
  delay(20);
  pump_off();
  TEST_ASSERT_FALSE(outputHigh());
  TEST_ASSERT_TRUE(pump_startTimed(200));
  delay(60);
  TEST_ASSERT_TRUE(outputHigh());
  delay(170);
  TEST_ASSERT_FALSE(outputHigh());
}

void maximum_run_cuts_off_while_application_is_idle() {
  TEST_ASSERT_TRUE(pump_startTimed(10000));
  delay(10100);
  TEST_ASSERT_FALSE(outputHigh());
  TEST_ASSERT_FALSE(pump_isRunning());
}

void ota_interlock_stops_and_stays_locked() {
  TEST_ASSERT_TRUE(pump_startTimed(1000));
  pump_inhibit();
  TEST_ASSERT_FALSE(outputHigh());
  TEST_ASSERT_FALSE(pump_isRunning());
  TEST_ASSERT_FALSE(pump_startTimed(50));
  pump_begin();
  TEST_ASSERT_FALSE(pump_startTimed(50));
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(rejects_start_before_initialization);
  RUN_TEST(initialization_failure_is_fail_closed);
  RUN_TEST(timer_start_failure_is_fail_closed);
  RUN_TEST(validates_duration_and_initializes_once);
  RUN_TEST(cuts_off_without_scheduler_or_cooperative_yield);
  RUN_TEST(busy_start_cannot_extend_deadline);
  RUN_TEST(old_deadline_cannot_stop_restarted_run);
  RUN_TEST(maximum_run_cuts_off_while_application_is_idle);
  RUN_TEST(ota_interlock_stops_and_stays_locked);
  UNITY_END();
}

void loop() { delay(1000); }
