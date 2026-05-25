#pragma once

enum UIScreen {
  UI_SCREEN_STATUS,
  UI_SCREEN_GROW,
  UI_SCREEN_FAN,
  UI_SCREEN_CLIMATE,
  UI_SCREEN_LIGHT,
  UI_SCREEN_PUMP,
  UI_SCREEN_SENSORS,
  UI_SCREEN_DIAG,
  UI_SCREEN_SYSTEM,
  UI_SCREEN_COUNT
};

void ui_begin();
void ui_loop();

void ui_rotate(int delta);
void ui_next();
void ui_click();
void ui_longClick();

int ui_getSelectedIndex();
int ui_getScreen();
int ui_getClimateSettingIndex();
int ui_getLightSettingIndex();
int ui_getPumpSettingIndex();
bool ui_isEditing();
bool ui_settingsReady();
const char* ui_getScreenName();
void ui_applyGrowModeDefaults();

// ===== LIGHT =====
bool ui_isLightAuto();
bool ui_isLightManualOn();
bool ui_isLightManualOff();
bool ui_isLightOff();
const char* ui_getLightModeName();
void ui_setLightAuto();
void ui_setLightManualOn();
void ui_setLightManualOff();

// ===== FAN =====
bool ui_isFanManual();
int ui_getFanManualValue();
const char* ui_getFanModeName();
void ui_setFanAuto();
void ui_setFanManual(int percent);
