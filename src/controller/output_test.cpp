#include "controller/output_test.h"

#include <Arduino.h>

#include "controller/equipment.h"
#include "controller/outputs.h"
#include "controller/timed_digital.h"
#include "modules/fan.h"
#include "modules/rest_mode.h"
#include "modules/wifi_ota.h"

namespace founderOutputTest {
static_assert(DurationMs <= founderDigital::MaxRuntimeMs, "Test exceeds digital cutoff");
static constexpr unsigned FanStart = 5;
static unsigned activePort = PortCount;
static uint32_t startedAt = 0;

bool available() {
  return restMode_isEnabled() && founderOutputs::ready() && founderEquipment::settingsReady() &&
         !founderOutputs::inhibited() && !wifiOTA_requiresSetup() && !wifiOTA_isTesting() &&
         !wifiOTA_setupActive();
}

bool running() {
  return activePort < PortCount;
}

unsigned remainingMs() {
  const uint32_t elapsed = millis() - startedAt;
  return running() && elapsed < DurationMs ? DurationMs - elapsed : 0;
}

bool start(unsigned port, unsigned percent) {
  if (running() || !available() || port >= PortCount || percent == 0 || percent > 100) {
    return false;
  }
  const bool started = port < FanStart ? founderOutputs::startTest(port, percent, DurationMs)
                                     : fan_startTest(port - FanStart + 1, percent);
  if (started) {
    activePort = port;
    startedAt = millis();
  }
  return started;
}

bool setLevel(unsigned percent) {
  loop();
  if (!running()) {
    return false;
  }
  if (percent == 0) {
    stop();
    return true;
  }
  return activePort < FanStart ? founderOutputs::setTestLevel(percent) : fan_setTestLevel(percent);
}

void stop() {
  if (!running()) {
    return;
  }
  if (activePort < FanStart) {
    founderOutputs::stopTest();
  } else {
    fan_stopTest();
  }
  activePort = PortCount;
}

void loop() {
  if (!running()) {
    return;
  }
  const bool driverActive = activePort < FanStart ? founderOutputs::testing() : fan_testing();
  if (!available() || !driverActive || remainingMs() == 0) {
    stop();
  }
}
}  // namespace founderOutputTest
