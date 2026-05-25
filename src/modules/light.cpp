#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "modules/grow_mode.h"
#include "modules/time_sync.h"
#include "modules/ui.h"
#include "modules/light.h"

static Preferences lightSettings;
static bool settingsReady = false;
static bool settingsDirty = false;

static int lightOnHour = LIGHT_ON_HOUR;
static int lightOffHour = LIGHT_OFF_HOUR;

static int wrapHour(int hour) {
  while (hour < 0) hour += 24;
  while (hour > 23) hour -= 24;
  return hour;
}

static bool isAutoLightOn() {
  if (!isTimeSynced()) return false;

  int hour = getHour();

  if (lightOnHour == lightOffHour) return true;

  if (lightOnHour < lightOffHour) {
    return hour >= lightOnHour && hour < lightOffHour;
  }

  return hour >= lightOnHour || hour < lightOffHour;
}

static void loadLightSettings() {
  settingsReady = lightSettings.begin("vgrow-light", false);

  if (!settingsReady) {
    Serial.println("Light settings unavailable");
    return;
  }

  lightOnHour = wrapHour(lightSettings.getInt("onH", LIGHT_ON_HOUR));
  lightOffHour = wrapHour(lightSettings.getInt("offH", LIGHT_OFF_HOUR));
  Serial.println("Light settings loaded");
}

void light_begin() {
  pinMode(PIN_LIGHT, OUTPUT);
  loadLightSettings();
}

void light_loop() {
  if (growMode_isGermination()) {
    digitalWrite(PIN_LIGHT, LOW);
    return;
  }

  if (ui_isLightManualOn()) {
    digitalWrite(PIN_LIGHT, HIGH);
    return;
  }

  if (ui_isLightManualOff()) {
    digitalWrite(PIN_LIGHT, LOW);
    return;
  }

  digitalWrite(PIN_LIGHT, isAutoLightOn() ? HIGH : LOW);
}

bool isLightOn() {
  if (growMode_isGermination()) return false;

  if (ui_isLightManualOn()) return true;
  if (ui_isLightManualOff()) return false;

  return isAutoLightOn();
}

int light_getOnHour() {
  return lightOnHour;
}

int light_getOffHour() {
  return lightOffHour;
}

const char* light_getReasonName() {
  if (growMode_isGermination()) return "GERM OFF";
  if (ui_isLightManualOn()) return "MAN ON";
  if (ui_isLightManualOff()) return "MAN OFF";
  if (!isTimeSynced()) return "TIME WAIT";
  return isAutoLightOn() ? "SCHED ON" : "SCHED OFF";
}

const char* light_getSettingName(int index) {
  switch (index) {
    case LIGHT_SETTING_MODE: return "Mode";
    case LIGHT_SETTING_ON_HOUR: return "On";
    case LIGHT_SETTING_OFF_HOUR: return "Off";
    default: return "Light";
  }
}

bool light_settingsReady() {
  return settingsReady;
}

bool light_setSchedule(int onHour, int offHour) {
  if (!settingsReady) return false;
  if (onHour < 0 || onHour > 23 || offHour < 0 || offHour > 23) return false;

  lightOnHour = onHour;
  lightOffHour = offHour;
  lightSettings.putInt("onH", lightOnHour);
  lightSettings.putInt("offH", lightOffHour);
  settingsDirty = false;
  Serial.println("Light schedule saved");
  return true;
}

void light_adjustSetting(int index, int delta) {
  if (delta == 0) return;

  if (index == LIGHT_SETTING_ON_HOUR) {
    lightOnHour = wrapHour(lightOnHour + delta);
    settingsDirty = true;
  }

  if (index == LIGHT_SETTING_OFF_HOUR) {
    lightOffHour = wrapHour(lightOffHour + delta);
    settingsDirty = true;
  }
}

void light_saveSettings() {
  if (!settingsReady || !settingsDirty) return;

  lightSettings.putInt("onH", lightOnHour);
  lightSettings.putInt("offH", lightOffHour);
  settingsDirty = false;
  Serial.println("Light settings saved");
}
