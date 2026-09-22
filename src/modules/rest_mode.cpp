#include <Arduino.h>
#include <Preferences.h>

#include "modules/rest_mode.h"

static Preferences restSettings;
static bool settingsReady = false;
static bool restEnabled = false;
static bool persistedEnabled = false;

void restMode_begin() {
  settingsReady = restSettings.begin("vgrow-rest", false);

  if (!settingsReady) {
    Serial.println("Rest mode settings unavailable");
    return;
  }

  restEnabled = restSettings.getBool("enabled", false);
  persistedEnabled = restEnabled;
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
  // Enter safety immediately, even if storage is unavailable. Leaving it must
  // first persist successfully. Track RAM-only changes so they are saved later.
  if (enabled) restEnabled = true;
  if (saveNow) {
    if (!settingsReady) return false;
    if (persistedEnabled != enabled) {
      if (restSettings.putBool("enabled", enabled) != 1) return false;
      persistedEnabled = enabled;
    }
  }
  restEnabled = enabled;
  return !saveNow || settingsReady;
}

const char* restMode_getName() {
  return restEnabled ? "REST" : "OFF";
}

const char* restMode_getReasonName() {
  return restEnabled ? "REST MODE" : "NORMAL";
}
