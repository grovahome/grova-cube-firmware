#include <Arduino.h>
#include "config.h"

void pump_begin() {
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);
}

void pump_on() {
  digitalWrite(PIN_PUMP, HIGH);
}

void pump_off() {
  digitalWrite(PIN_PUMP, LOW);
}