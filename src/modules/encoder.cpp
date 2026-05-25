#include <Arduino.h>
#include "config.h"
#include "modules/display.h"
#include "modules/ui.h"

// ===== STATE =====
int lastCLK;
unsigned long lastMove = 0;

bool lastButton = HIGH;
unsigned long buttonDownAt = 0;
bool longClickHandled = false;

static const unsigned long MOVE_DEBOUNCE_MS = 120;
static const unsigned long CLICK_DEBOUNCE_MS = 40;
static const unsigned long LONG_CLICK_MS = 800;

void encoder_begin() {

  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  lastCLK = digitalRead(ENCODER_CLK);

  Serial.println("Encoder v2 ready");
}

void encoder_loop() {

  // Rotation
  int clk = digitalRead(ENCODER_CLK);

  if (clk != lastCLK && clk == LOW) {

    if (millis() - lastMove > MOVE_DEBOUNCE_MS) {
      int direction = (digitalRead(ENCODER_DT) != clk) ? 1 : -1;

      if (display_wakeFromUser()) {
        lastMove = millis();
        lastCLK = clk;
        return;
      }

      ui_rotate(direction);
      lastMove = millis();
    }
  }

  lastCLK = clk;

  // Button
  bool btn = digitalRead(ENCODER_SW);

  if (btn == LOW && lastButton == HIGH) {
    if (display_wakeFromUser()) {
      lastButton = btn;
      buttonDownAt = millis();
      longClickHandled = true;
      return;
    }

    buttonDownAt = millis();
    longClickHandled = false;
  }

  if (btn == LOW && !longClickHandled && millis() - buttonDownAt >= LONG_CLICK_MS) {
    ui_longClick();
    longClickHandled = true;
  }

  if (btn == HIGH && lastButton == LOW) {
    unsigned long pressTime = millis() - buttonDownAt;

    if (!longClickHandled && pressTime >= CLICK_DEBOUNCE_MS) {
      ui_click();
    }
  }

  lastButton = btn;
}
