#include <Arduino.h>
#include <string.h>

#include "config.h"
#include "modules/outputs.h"

static bool aux12vOn = false;
static bool aux5vOn = false;

static bool validPin(int pin) {
  return pin >= 0;
}

static void writeOutput(int pin, bool on) {
  if (!validPin(pin)) return;
  digitalWrite(pin, on ? HIGH : LOW);
}

void outputs_begin() {
  if (validPin(PIN_AUX_12V)) {
    pinMode(PIN_AUX_12V, OUTPUT);
    writeOutput(PIN_AUX_12V, false);
  }

  if (validPin(PIN_AUX_5V)) {
    pinMode(PIN_AUX_5V, OUTPUT);
    writeOutput(PIN_AUX_5V, false);
  }

  aux12vOn = false;
  aux5vOn = false;
}

bool outputs_setAux12v(bool on) {
  if (!validPin(PIN_AUX_12V)) return false;
  aux12vOn = on;
  writeOutput(PIN_AUX_12V, aux12vOn);
  return true;
}

bool outputs_setAux5v(bool on) {
  if (!validPin(PIN_AUX_5V)) return false;
  aux5vOn = on;
  writeOutput(PIN_AUX_5V, aux5vOn);
  return true;
}

bool outputs_setByName(const char* output, bool on) {
  if (strcmp(output, "AUX_12V") == 0 || strcmp(output, "AUX12V") == 0 || strcmp(output, "12V") == 0) {
    return outputs_setAux12v(on);
  }

  if (strcmp(output, "AUX_5V") == 0 || strcmp(output, "AUX5V") == 0 || strcmp(output, "5V") == 0) {
    return outputs_setAux5v(on);
  }

  return false;
}

bool outputs_isAux12vOn() {
  return aux12vOn;
}

bool outputs_isAux5vOn() {
  return aux5vOn;
}

bool outputs_hasAux12v() {
  return validPin(PIN_AUX_12V);
}

bool outputs_hasAux5v() {
  return validPin(PIN_AUX_5V);
}
