#pragma once

enum LightSetting {
  LIGHT_SETTING_MODE,
  LIGHT_SETTING_ON_HOUR,
  LIGHT_SETTING_OFF_HOUR,
  LIGHT_SETTING_COUNT
};

void light_begin();
void light_loop();

bool isLightOn();
int light_getOnHour();
int light_getOffHour();
const char* light_getReasonName();
const char* light_getSettingName(int index);
bool light_settingsReady();
bool light_setSchedule(int onHour, int offHour);
void light_adjustSetting(int index, int delta);
void light_saveSettings();
