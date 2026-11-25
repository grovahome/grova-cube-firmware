#include "controller/inputs.h"

#include <Arduino.h>

#include "boards/grova_core_s3.h"
namespace founderInputs {
static constexpr uint32_t TurnIntervalMs = 140;
static portMUX_TYPE encoderLock = portMUX_INITIALIZER_UNLOCKED;
static int detentPhase = 0, lastPhase = 0, phaseSteps = 0;
static volatile int pendingTurn = 0;
static bool lastButton = true, configHandled = false;
static uint32_t configAt = 0, moveAt = 0, buttonAt = 0;

static int IRAM_ATTR encoderPhase() {
  return (digitalRead(founder::ENC_CLK) << 1) | digitalRead(founder::ENC_DT);
}

static void IRAM_ATTR onEncoderChange() {
  // Capture both contacts even while the main loop is drawing an OLED frame.
  portENTER_CRITICAL_ISR(&encoderLock);
  const int phase = encoderPhase();
  const int changed = phase ^ lastPhase;
  if (changed) {
    if (changed == 3) {
      phaseSteps = 0;  // Both bits changed. The direction is ambiguous.
    } else {
      // Bounce retraces an edge and cancels its contribution.
      phaseSteps += (lastPhase >> 1) == (phase & 1) ? 1 : -1;
    }
    if (phase == detentPhase) {
      if (phaseSteps == 4 || phaseSteps == -4) {
        pendingTurn = phaseSteps / 4;
      }
      phaseSteps = 0;
    }
    lastPhase = phase;
  }
  portEXIT_CRITICAL_ISR(&encoderLock);
}

void begin() {
  pinMode(founder::ENC_CLK, INPUT_PULLUP);
  pinMode(founder::ENC_DT, INPUT_PULLUP);
  pinMode(founder::ENC_SW, INPUT_PULLUP);
  pinMode(founder::CONFIG_BUTTON, INPUT);
  pinMode(founder::RTC_INT, INPUT);
  detentPhase = lastPhase = encoderPhase();
  moveAt = millis() - TurnIntervalMs;
  lastButton = digitalRead(founder::ENC_SW);
  attachInterrupt(founder::ENC_CLK, onEncoderChange, CHANGE);
  attachInterrupt(founder::ENC_DT, onEncoderChange, CHANGE);
}
Event poll() {
  Event event;
  const uint32_t now = millis();
  // Keep one pending direction. Several detents between two polls must not
  // become a sequence of delayed menu jumps.
  portENTER_CRITICAL(&encoderLock);
  const int turn = pendingTurn;
  pendingTurn = 0;
  portEXIT_CRITICAL(&encoderLock);
  const bool button = digitalRead(founder::ENC_SW);
  if (turn && button && lastButton && now - moveAt >= TurnIntervalMs) {
    event.turn = turn;
    moveAt = now;
  }
  if (!button && lastButton) {
    buttonAt = now;
  }
  if (button && !lastButton) {
    const uint32_t duration = now - buttonAt;
    event.pressed = duration >= 40 && duration < 700;
    event.held = duration >= 700;
  }
  lastButton = button;
  if (digitalRead(founder::CONFIG_BUTTON) == LOW) {
    if (!configAt) {
      configAt = now;
    }
    if (!configHandled && now - configAt >= 5000) {
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
