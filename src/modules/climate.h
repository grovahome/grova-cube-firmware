#pragma once

enum ClimateSetting {
  CLIMATE_DAY_TEMP,
  CLIMATE_NIGHT_TEMP,
  CLIMATE_DAY_HUM,
  CLIMATE_NIGHT_HUM,
  CLIMATE_SETTING_COUNT
};

void climate_begin();
void climate_loop();

float getTargetTemp();
float getTargetHum();

float climate_getDayTemp();
float climate_getNightTemp();
float climate_getDayHum();
float climate_getNightHum();
float climate_getSettingValue(int index);
const char* climate_getSettingName(int index);
const char* climate_getSettingUnit(int index);
bool climate_settingsReady();
void climate_adjustSetting(int index, int delta);
bool climate_setTargets(float dayTemp, float nightTemp, int dayHumValue, int nightHumValue);
void climate_saveSettings();
