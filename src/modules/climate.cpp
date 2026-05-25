#include <Arduino.h>
#include <Preferences.h>
#include "modules/light.h"
#include "config.h"
#include "modules/climate.h"

static constexpr int DEFAULT_DAY_TEMP_X10 = 230;
static constexpr int DEFAULT_NIGHT_TEMP_X10 = 220;
static constexpr int DEFAULT_DAY_HUM = 65;
static constexpr int DEFAULT_NIGHT_HUM = 70;

static constexpr int TEMP_MIN_X10 = 160;
static constexpr int TEMP_MAX_X10 = 320;
static constexpr int HUM_MIN = 30;
static constexpr int HUM_MAX = 90;

static Preferences climateSettings;
static bool settingsReady = false;
static bool settingsDirty = false;

static int dayTempX10 = DEFAULT_DAY_TEMP_X10;
static int nightTempX10 = DEFAULT_NIGHT_TEMP_X10;
static int dayHum = DEFAULT_DAY_HUM;
static int nightHum = DEFAULT_NIGHT_HUM;

static float targetTemp = 23.0;
static float targetHum = 65.0;

static int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static float x10ToFloat(int value) {
  return value / 10.0;
}

static void loadSettings() {
  settingsReady = climateSettings.begin("vgrow-clim", false);

  if (!settingsReady) {
    Serial.println("Climate settings unavailable");
    return;
  }

  dayTempX10 = clampInt(climateSettings.getInt("dayT", DEFAULT_DAY_TEMP_X10), TEMP_MIN_X10, TEMP_MAX_X10);
  nightTempX10 = clampInt(climateSettings.getInt("nightT", DEFAULT_NIGHT_TEMP_X10), TEMP_MIN_X10, TEMP_MAX_X10);
  dayHum = clampInt(climateSettings.getInt("dayH", DEFAULT_DAY_HUM), HUM_MIN, HUM_MAX);
  nightHum = clampInt(climateSettings.getInt("nightH", DEFAULT_NIGHT_HUM), HUM_MIN, HUM_MAX);

  Serial.println("Climate settings loaded");
}

void climate_begin() {
  loadSettings();
}

void climate_loop() {
  if (isLightOn()) {
    targetTemp = climate_getDayTemp();
    targetHum = climate_getDayHum();
  } else {
    targetTemp = climate_getNightTemp();
    targetHum = climate_getNightHum();
  }
}

float getTargetTemp() { return targetTemp; }
float getTargetHum() { return targetHum; }

float climate_getDayTemp() { return x10ToFloat(dayTempX10); }
float climate_getNightTemp() { return x10ToFloat(nightTempX10); }
float climate_getDayHum() { return dayHum; }
float climate_getNightHum() { return nightHum; }

float climate_getSettingValue(int index) {
  switch (index) {
    case CLIMATE_DAY_TEMP: return climate_getDayTemp();
    case CLIMATE_NIGHT_TEMP: return climate_getNightTemp();
    case CLIMATE_DAY_HUM: return climate_getDayHum();
    case CLIMATE_NIGHT_HUM: return climate_getNightHum();
    default: return 0;
  }
}

const char* climate_getSettingName(int index) {
  switch (index) {
    case CLIMATE_DAY_TEMP: return "Day T";
    case CLIMATE_NIGHT_TEMP: return "Night T";
    case CLIMATE_DAY_HUM: return "Day H";
    case CLIMATE_NIGHT_HUM: return "Night H";
    default: return "Value";
  }
}

const char* climate_getSettingUnit(int index) {
  if (index == CLIMATE_DAY_TEMP || index == CLIMATE_NIGHT_TEMP) return "C";
  return "%";
}

bool climate_settingsReady() {
  return settingsReady;
}

void climate_adjustSetting(int index, int delta) {
  if (delta == 0) return;

  switch (index) {
    case CLIMATE_DAY_TEMP:
      dayTempX10 = clampInt(dayTempX10 + (delta * 5), TEMP_MIN_X10, TEMP_MAX_X10);
      break;
    case CLIMATE_NIGHT_TEMP:
      nightTempX10 = clampInt(nightTempX10 + (delta * 5), TEMP_MIN_X10, TEMP_MAX_X10);
      break;
    case CLIMATE_DAY_HUM:
      dayHum = clampInt(dayHum + delta, HUM_MIN, HUM_MAX);
      break;
    case CLIMATE_NIGHT_HUM:
      nightHum = clampInt(nightHum + delta, HUM_MIN, HUM_MAX);
      break;
    default:
      return;
  }

  settingsDirty = true;
}

bool climate_setTargets(float dayTemp, float nightTemp, int dayHumValue, int nightHumValue) {
  int newDayTempX10 = clampInt(lroundf(dayTemp * 10.0), TEMP_MIN_X10, TEMP_MAX_X10);
  int newNightTempX10 = clampInt(lroundf(nightTemp * 10.0), TEMP_MIN_X10, TEMP_MAX_X10);
  int newDayHum = clampInt(dayHumValue, HUM_MIN, HUM_MAX);
  int newNightHum = clampInt(nightHumValue, HUM_MIN, HUM_MAX);

  bool changed =
    dayTempX10 != newDayTempX10 ||
    nightTempX10 != newNightTempX10 ||
    dayHum != newDayHum ||
    nightHum != newNightHum;

  dayTempX10 = newDayTempX10;
  nightTempX10 = newNightTempX10;
  dayHum = newDayHum;
  nightHum = newNightHum;

  if (changed) settingsDirty = true;
  climate_loop();
  climate_saveSettings();
  return settingsReady;
}

void climate_saveSettings() {
  if (!settingsReady || !settingsDirty) return;

  climateSettings.putInt("dayT", dayTempX10);
  climateSettings.putInt("nightT", nightTempX10);
  climateSettings.putInt("dayH", dayHum);
  climateSettings.putInt("nightH", nightHum);

  settingsDirty = false;
  Serial.println("Climate settings saved");
}
