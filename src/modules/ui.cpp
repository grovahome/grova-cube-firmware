#include <Arduino.h>
#include <Preferences.h>
#include "modules/climate.h"
#include "modules/grow_mode.h"
#include "modules/light.h"
#include "modules/ui.h"
#include "modules/pump_scheduler.h"

enum Mode { AUTO, MANUAL_ON, MANUAL_OFF, MANUAL };

struct UIState {
  int screen = UI_SCREEN_STATUS;
  int climateSettingIndex = CLIMATE_DAY_TEMP;
  int lightSettingIndex = LIGHT_SETTING_MODE;
  int pumpSettingIndex = PUMP_SETTING_TEST;
  bool editing = false;

  Mode lightMode = AUTO;
  Mode fanMode = AUTO;

  int fanManualValue = 50;
};

static UIState state;
static Preferences settings;
static bool settingsReady = false;
static bool settingsDirty = false;

static int clampValue(int value, int minValue, int maxValue);

static Mode cleanLightMode(uint8_t value) {
  if (value == MANUAL_ON) return MANUAL_ON;
  if (value == MANUAL_OFF) return MANUAL_OFF;
  return AUTO;
}

static Mode cleanFanMode(uint8_t value) {
  return value == MANUAL ? MANUAL : AUTO;
}

static void saveSettings() {
  if (!settingsReady || !settingsDirty) return;

  settings.putUChar("light", static_cast<uint8_t>(state.lightMode));
  settings.putUChar("fan", static_cast<uint8_t>(state.fanMode));
  settings.putUChar("fanPct", static_cast<uint8_t>(state.fanManualValue));

  settingsDirty = false;
  Serial.println("Settings saved");
}

static void markSettingsDirty() {
  settingsDirty = true;
}

static void setLightMode(Mode mode, bool saveNow) {
  mode = cleanLightMode(static_cast<uint8_t>(mode));

  if (state.lightMode == mode) return;

  state.lightMode = mode;
  markSettingsDirty();
  if (saveNow) saveSettings();
}

static void setFanMode(Mode mode, bool saveNow) {
  mode = cleanFanMode(static_cast<uint8_t>(mode));

  if (state.fanMode == mode) return;

  state.fanMode = mode;
  markSettingsDirty();
  if (saveNow) saveSettings();
}

static void setFanManualValue(int value, bool saveNow) {
  value = clampValue(value, 0, 100);

  if (state.fanManualValue == value) return;

  state.fanManualValue = value;
  markSettingsDirty();
  if (saveNow) saveSettings();
}

static void applyGrowModeDefaults(bool saveNow) {
  if (growMode_isGermination() || growMode_isGrowth() || growMode_isHarvest()) {
    setLightMode(AUTO, false);
    setFanMode(AUTO, false);
  }

  if (saveNow) saveSettings();
}

void ui_applyGrowModeDefaults() {
  applyGrowModeDefaults(true);
}

// =====================
void ui_begin() {
  settingsReady = settings.begin("vgrow", false);

  if (settingsReady) {
    state.lightMode = cleanLightMode(settings.getUChar("light", AUTO));
    state.fanMode = cleanFanMode(settings.getUChar("fan", AUTO));
    state.fanManualValue = clampValue(settings.getUChar("fanPct", 50), 0, 100);
    applyGrowModeDefaults(false);
    Serial.println("Settings loaded");
  } else {
    Serial.println("Settings unavailable");
  }

  Serial.println("UI gestartet");
}

void ui_loop() {}

// =====================
static int clampValue(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static int wrapScreen(int screen) {
  if (screen < 0) return UI_SCREEN_COUNT - 1;
  if (screen >= UI_SCREEN_COUNT) return 0;
  return screen;
}

void ui_rotate(int delta) {
  if (delta == 0) return;

  if (state.editing && state.screen == UI_SCREEN_GROW) {
    growMode_adjust(delta);
    return;
  }

  if (state.editing && state.screen == UI_SCREEN_FAN) {
    setFanMode(MANUAL, false);
    setFanManualValue(state.fanManualValue + (delta * 5), false);
    return;
  }

  if (state.editing && state.screen == UI_SCREEN_CLIMATE) {
    climate_adjustSetting(state.climateSettingIndex, delta);
    return;
  }

  if (state.editing && state.screen == UI_SCREEN_LIGHT) {
    light_adjustSetting(state.lightSettingIndex, delta);
    return;
  }

  if (state.editing && state.screen == UI_SCREEN_PUMP) {
    pumpScheduler_adjustSetting(state.pumpSettingIndex, delta);
    return;
  }

  state.screen = wrapScreen(state.screen + delta);
}

void ui_next() {
  ui_rotate(1);
}

void ui_click() {

  if (state.screen == UI_SCREEN_GROW) {
    if (state.editing) {
      state.editing = false;
      growMode_save();
      applyGrowModeDefaults(true);
    } else {
      growMode_adjust(1);
      growMode_save();
      applyGrowModeDefaults(true);
    }
    return;
  }

  if (state.screen == UI_SCREEN_FAN) {
    state.editing = !state.editing;

    if (state.editing) {
      setFanMode(MANUAL, false);
    } else {
      saveSettings();
    }
    return;
  }

  if (state.screen == UI_SCREEN_CLIMATE) {
    if (state.editing) {
      state.editing = false;
      climate_saveSettings();
    } else {
      state.climateSettingIndex = (state.climateSettingIndex + 1) % CLIMATE_SETTING_COUNT;
    }
    return;
  }

  if (state.screen == UI_SCREEN_LIGHT) {
    if (state.editing) {
      state.editing = false;
      light_saveSettings();
      return;
    }

    state.lightSettingIndex = (state.lightSettingIndex + 1) % LIGHT_SETTING_COUNT;
    return;
  }

  if (state.screen == UI_SCREEN_PUMP) {
    if (state.editing) {
      state.editing = false;
      pumpScheduler_saveSettings();
      return;
    }

    state.pumpSettingIndex = (state.pumpSettingIndex + 1) % PUMP_SETTING_COUNT;
    return;
  }
}

void ui_longClick() {
  if (state.editing) {
    state.editing = false;
    if (state.screen == UI_SCREEN_CLIMATE) {
      climate_saveSettings();
    } else if (state.screen == UI_SCREEN_GROW) {
      growMode_save();
      applyGrowModeDefaults(true);
    } else if (state.screen == UI_SCREEN_LIGHT) {
      light_saveSettings();
    } else if (state.screen == UI_SCREEN_PUMP) {
      pumpScheduler_saveSettings();
    } else {
      saveSettings();
    }
    return;
  }

  if (state.screen == UI_SCREEN_FAN) {
    setFanMode((state.fanMode == AUTO) ? MANUAL : AUTO, true);
    return;
  }

  if (state.screen == UI_SCREEN_GROW) {
    state.editing = true;
    return;
  }

  if (state.screen == UI_SCREEN_CLIMATE) {
    state.editing = true;
    return;
  }

  if (state.screen == UI_SCREEN_LIGHT) {
    if (state.lightSettingIndex == LIGHT_SETTING_MODE) {
      if (state.lightMode == AUTO) {
        setLightMode(MANUAL_OFF, true);
      } else if (state.lightMode == MANUAL_OFF) {
        setLightMode(MANUAL_ON, true);
      } else {
        setLightMode(AUTO, true);
      }
    } else {
      state.editing = true;
    }
    return;
  }

  if (state.screen == UI_SCREEN_PUMP) {
    if (state.pumpSettingIndex == PUMP_SETTING_TEST) {
      if (pumpScheduler_isRunning()) {
        pumpScheduler_manualStop();
      } else {
        pumpScheduler_manualStart();
      }
    } else {
      state.editing = true;
    }
  }
}

// =====================
// LIGHT STATES
// =====================
bool ui_isLightAuto() {
  return state.lightMode == AUTO;
}

bool ui_isLightManualOn() {
  return state.lightMode == MANUAL_ON;
}

bool ui_isLightManualOff() {
  return state.lightMode == MANUAL_OFF;
}

bool ui_isLightOff() {
  return state.lightMode == MANUAL_OFF;
}

const char* ui_getLightModeName() {
  if (state.lightMode == MANUAL_ON) return "MAN ON";
  if (state.lightMode == MANUAL_OFF) return "MAN OFF";
  return "AUTO";
}

void ui_setLightAuto() {
  setLightMode(AUTO, true);
}

void ui_setLightManualOn() {
  setLightMode(MANUAL_ON, true);
}

void ui_setLightManualOff() {
  setLightMode(MANUAL_OFF, true);
}

// =====================
// FAN
// =====================
bool ui_isFanManual() {
  return state.fanMode == MANUAL;
}

int ui_getFanManualValue() {
  return state.fanManualValue;
}

const char* ui_getFanModeName() {
  return state.fanMode == MANUAL ? "MANUAL" : "AUTO";
}

void ui_setFanAuto() {
  setFanMode(AUTO, true);
}

void ui_setFanManual(int percent) {
  setFanMode(MANUAL, false);
  setFanManualValue(percent, true);
}

// =====================
int ui_getSelectedIndex() {
  return state.screen;
}

int ui_getScreen() {
  return state.screen;
}

int ui_getClimateSettingIndex() {
  return state.climateSettingIndex;
}

int ui_getLightSettingIndex() {
  return state.lightSettingIndex;
}

int ui_getPumpSettingIndex() {
  return state.pumpSettingIndex;
}

bool ui_isEditing() {
  return state.editing;
}

bool ui_settingsReady() {
  return settingsReady;
}

const char* ui_getScreenName() {
  switch (state.screen) {
    case UI_SCREEN_STATUS: return "STATUS";
    case UI_SCREEN_GROW: return "GROW";
    case UI_SCREEN_FAN: return "FAN";
    case UI_SCREEN_CLIMATE: return "CLIMATE";
    case UI_SCREEN_LIGHT: return "LIGHT";
    case UI_SCREEN_PUMP: return "PUMP";
    case UI_SCREEN_SENSORS: return "SENSORS";
    case UI_SCREEN_DIAG: return "DIAG";
    case UI_SCREEN_SYSTEM: return "SYSTEM";
    default: return "MENU";
  }
}
