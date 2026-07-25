#include <Arduino.h>
#include <Preferences.h>

#include "modules/rest_mode.h"

static Preferences restSettings;
static bool settingsReady = false;
static bool restEnabled = false;

void restMode_begin() {
  settingsReady = restSettings.begin("vgrow-rest", false);

  if (!settingsReady) {
    Serial.println("Rest mode settings unavailable");
    return;
  }

  restEnabled = restSettings.getBool("enabled", false);
  Serial.print("Rest mode loaded: ");
  Serial.println(restMode_getName());
}

bool restMode_isEnabled() {
  return restEnabled;
}

bool restMode_settingsReady() {
  return settingsReady;
}

bool restMode_setEnabled(bool enabled, bool saveNow) {
  if (saveNow && !settingsReady) return false;
  if (restEnabled == enabled) return true;

  restEnabled = enabled;
  if (saveNow && settingsReady) {
    restSettings.putBool("enabled", restEnabled);
    Serial.print("Rest mode saved: ");
    Serial.println(restMode_getName());
  }

  return settingsReady;
}

const char* restMode_getName() {
  return restEnabled ? "REST" : "OFF";
}

const char* restMode_getReasonName() {
  return restEnabled ? "REST MODE" : "NORMAL";
}
