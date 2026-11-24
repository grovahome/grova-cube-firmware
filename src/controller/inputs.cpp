#include "controller/inputs.h"

#include <Arduino.h>

#include "boards/grova_core_s3.h"
namespace founderInputs {
static int lastClock = HIGH;
static bool lastButton = true, configHandled = false;
static uint32_t configAt = 0, moveAt = 0, buttonAt = 0;
void begin() {
  pinMode(founder::ENC_CLK, INPUT_PULLUP);
  pinMode(founder::ENC_DT, INPUT_PULLUP);
  pinMode(founder::ENC_SW, INPUT_PULLUP);
  pinMode(founder::CONFIG_BUTTON, INPUT);
  pinMode(founder::RTC_INT, INPUT);
  lastClock = digitalRead(founder::ENC_CLK);
  lastButton = digitalRead(founder::ENC_SW);
}
Event poll() {
  Event event;
  int clk = digitalRead(founder::ENC_CLK);
  if (clk != lastClock && clk == LOW && millis() - moveAt >= 80) {
    event.turn = digitalRead(founder::ENC_DT) != clk ? 1 : -1;
    moveAt = millis();
  }
  lastClock = clk;
  bool button = digitalRead(founder::ENC_SW);
  if (!button && lastButton) {
    buttonAt = millis();
  }
  if (button && !lastButton) {
    const uint32_t duration = millis() - buttonAt;
    event.pressed = duration >= 40 && duration < 700;
    event.held = duration >= 700;
  }
  lastButton = button;
  if (digitalRead(founder::CONFIG_BUTTON) == LOW) {
    if (!configAt) {
      configAt = millis();
    }
    if (!configHandled && millis() - configAt >= 5000) {
      event.requestSetup = true;
      configHandled = true;
    }
  } else {
    configAt = 0;
    configHandled = false;
  }
  return event;
}
}  // namespace founderInputs
