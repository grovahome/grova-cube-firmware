#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "modules/alarms.h"
#include "modules/climate.h"
#include "modules/fan.h"
#include "modules/rest_mode.h"
#include "modules/runtime_config.h"
#include "modules/sensors.h"
#include "modules/time_sync.h"

static bool isTimeWarningActive() {
  return !isTimeSynced() && millis() > WARN_TIME_SYNC_DELAY_MS;
}

const char* alarms_getPrimaryWarning(float temp, float hum) {
  if (sensors_hasFault()) return sensors_getStatusName();
  if (fan_hasTachoFault()) return "FAN TACH";
  if (isTimeWarningActive()) return "TIME SYNC";

  if (restMode_isEnabled()) return "OK";

  if (temp >= runtimeConfig_getTempMaxC()) return "TEMP HIGH";
  if (temp <= runtimeConfig_getTempMinC()) return "TEMP LOW";
  if (hum >= runtimeConfig_getHumMaxPct()) return "HUM HIGH";
  if (hum <= runtimeConfig_getHumMinPct()) return "HUM LOW";

  return "OK";
}

bool alarms_hasWarning(float temp, float hum) {
  return strcmp(alarms_getPrimaryWarning(temp, hum), "OK") != 0;
}
