#include <Arduino.h>
#include <Preferences.h>
#include "modules/grow_mode.h"

static Preferences growSettings;
static bool settingsReady = false;
static bool settingsDirty = false;
static GrowMode currentMode = GROW_MODE_GROWTH;

static GrowMode cleanMode(int value) {
  if (value == GROW_MODE_GERMINATION) return GROW_MODE_GERMINATION;
  if (value == GROW_MODE_HARVEST) return GROW_MODE_HARVEST;
  return GROW_MODE_GROWTH;
}

void growMode_begin() {
  settingsReady = growSettings.begin("vgrow-mode", false);

  if (!settingsReady) {
    Serial.println("Grow mode settings unavailable");
    return;
  }

  currentMode = cleanMode(growSettings.getUChar("mode", GROW_MODE_GROWTH));
  Serial.print("Grow mode loaded: ");
  Serial.println(growMode_getName());
}

GrowMode growMode_get() {
  return currentMode;
}

bool growMode_set(GrowMode mode, bool saveNow) {
  mode = cleanMode(static_cast<int>(mode));

  if (currentMode == mode) return true;

  currentMode = mode;
  settingsDirty = true;

  if (saveNow) growMode_save();
  return true;
}

const char* growMode_getName() {
  switch (currentMode) {
    case GROW_MODE_GERMINATION: return "GERM";
    case GROW_MODE_HARVEST: return "HARVEST";
    case GROW_MODE_GROWTH:
    default: return "GROWTH";
  }
}

const char* growMode_getEffectName() {
  switch (currentMode) {
    case GROW_MODE_GERMINATION: return "Light off";
    case GROW_MODE_HARVEST: return "Pump auto off";
    case GROW_MODE_GROWTH:
    default: return "Normal auto";
  }
}

bool growMode_settingsReady() {
  return settingsReady;
}

bool growMode_isGermination() {
  return currentMode == GROW_MODE_GERMINATION;
}

bool growMode_isGrowth() {
  return currentMode == GROW_MODE_GROWTH;
}

bool growMode_isHarvest() {
  return currentMode == GROW_MODE_HARVEST;
}

void growMode_adjust(int delta) {
  if (delta == 0) return;

  int next = static_cast<int>(currentMode) + delta;

  while (next < 0) next += GROW_MODE_COUNT;
  while (next >= GROW_MODE_COUNT) next -= GROW_MODE_COUNT;

  currentMode = cleanMode(next);
  settingsDirty = true;
}

void growMode_save() {
  if (!settingsReady || !settingsDirty) return;

  growSettings.putUChar("mode", static_cast<uint8_t>(currentMode));
  settingsDirty = false;

  Serial.print("Grow mode saved: ");
  Serial.println(growMode_getName());
}
