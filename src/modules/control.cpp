#include <Arduino.h>
#include <ctype.h>
#include <string.h>
#include "modules/climate.h"
#include "modules/control.h"
#include "modules/fan.h"
#include "modules/grow_mode.h"
#include "modules/light.h"
#include "modules/local_run.h"
#include "modules/outputs.h"
#include "modules/preset_store.h"
#include "modules/pump_scheduler.h"
#include "modules/rest_mode.h"
#include "modules/runtime_config.h"
#include "modules/time_sync.h"
#include "modules/ui.h"

static void normalizeToken(char* value) {
  for (size_t i = 0; value[i] != '\0'; i++) {
    if (value[i] == '-' || value[i] == ' ') value[i] = '_';
    value[i] = toupper(static_cast<unsigned char>(value[i]));
  }
}

static bool extractString(const String& body, const char* key, char* output, size_t outputSize) {
  if (outputSize == 0) return false;
  output[0] = '\0';

  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = body.indexOf('"', colonPos + 1);
  if (valueStart < 0) return false;

  int valueEnd = body.indexOf('"', valueStart + 1);
  if (valueEnd < 0) return false;

  size_t length = min(static_cast<size_t>(valueEnd - valueStart - 1), outputSize - 1);
  body.substring(valueStart + 1, valueStart + 1 + length).toCharArray(output, outputSize);
  return true;
}

static bool extractStringValue(const String& body, const char* key, String& output) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = body.indexOf('"', colonPos + 1);
  if (valueStart < 0) return false;

  int valueEnd = body.indexOf('"', valueStart + 1);
  if (valueEnd < 0) return false;

  output = body.substring(valueStart + 1, valueEnd);
  return true;
}

static bool extractInt(const String& body, const char* key, int& value) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = colonPos + 1;
  while (valueStart < static_cast<int>(body.length()) && isspace(body[valueStart])) valueStart++;

  int valueEnd = valueStart;
  while (valueEnd < static_cast<int>(body.length()) && (isdigit(body[valueEnd]) || body[valueEnd] == '-')) valueEnd++;

  if (valueEnd == valueStart) return false;

  value = body.substring(valueStart, valueEnd).toInt();
  return true;
}

static bool extractUnsignedLong(const String& body, const char* key, unsigned long& value) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = colonPos + 1;
  while (valueStart < static_cast<int>(body.length()) && isspace(body[valueStart])) valueStart++;

  int valueEnd = valueStart;
  while (valueEnd < static_cast<int>(body.length()) && isdigit(body[valueEnd])) valueEnd++;

  if (valueEnd == valueStart) return false;

  value = strtoul(body.substring(valueStart, valueEnd).c_str(), nullptr, 10);
  return true;
}

static bool extractUnsignedLongLong(const String& body, const char* key, unsigned long long& value) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = colonPos + 1;
  while (valueStart < static_cast<int>(body.length()) && isspace(body[valueStart])) valueStart++;

  int valueEnd = valueStart;
  while (valueEnd < static_cast<int>(body.length()) && isdigit(body[valueEnd])) valueEnd++;

  if (valueEnd == valueStart) return false;

  value = strtoull(body.substring(valueStart, valueEnd).c_str(), nullptr, 10);
  return true;
}

static bool extractFloat(const String& body, const char* key, float& value) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = colonPos + 1;
  while (valueStart < static_cast<int>(body.length()) && isspace(body[valueStart])) valueStart++;

  int valueEnd = valueStart;
  while (
    valueEnd < static_cast<int>(body.length()) &&
    (isdigit(body[valueEnd]) || body[valueEnd] == '-' || body[valueEnd] == '.')
  ) {
    valueEnd++;
  }

  if (valueEnd == valueStart) return false;

  value = body.substring(valueStart, valueEnd).toFloat();
  return true;
}

static bool extractBool(const String& body, const char* key, bool& value) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return false;

  int colonPos = body.indexOf(':', keyPos + needle.length());
  if (colonPos < 0) return false;

  int valueStart = colonPos + 1;
  while (valueStart < static_cast<int>(body.length()) && isspace(body[valueStart])) valueStart++;

  if (body.startsWith("true", valueStart)) {
    value = true;
    return true;
  }

  if (body.startsWith("false", valueStart)) {
    value = false;
    return true;
  }

  char token[12];
  if (extractString(body, key, token, sizeof(token))) {
    normalizeToken(token);
    if (strcmp(token, "ON") == 0 || strcmp(token, "TRUE") == 0 || strcmp(token, "1") == 0) {
      value = true;
      return true;
    }
    if (strcmp(token, "OFF") == 0 || strcmp(token, "FALSE") == 0 || strcmp(token, "0") == 0) {
      value = false;
      return true;
    }
  }

  return false;
}

static void makeResponse(String& responseJson, bool ok, const char* message) {
  responseJson = "{\"ok\":";
  responseJson += ok ? "true" : "false";
  responseJson += ",\"message\":\"";
  responseJson += message;
  responseJson += "\"}";
}

static bool setGrowModeByName(const char* mode) {
  if (strcmp(mode, "GERM") == 0 || strcmp(mode, "GERMINATION") == 0) {
    growMode_set(GROW_MODE_GERMINATION, true);
    ui_applyGrowModeDefaults();
    return true;
  }

  if (strcmp(mode, "GROWTH") == 0 || strcmp(mode, "GROW") == 0) {
    growMode_set(GROW_MODE_GROWTH, true);
    ui_applyGrowModeDefaults();
    return true;
  }

  if (strcmp(mode, "HARVEST") == 0) {
    growMode_set(GROW_MODE_HARVEST, true);
    ui_applyGrowModeDefaults();
    return true;
  }

  return false;
}

static bool setLightModeByName(const char* mode) {
  if (strcmp(mode, "AUTO") == 0) {
    ui_setLightAuto();
    return true;
  }

  if (strcmp(mode, "MAN_ON") == 0 || strcmp(mode, "ON") == 0) {
    ui_setLightManualOn();
    return true;
  }

  if (strcmp(mode, "MAN_OFF") == 0 || strcmp(mode, "OFF") == 0) {
    ui_setLightManualOff();
    return true;
  }

  return false;
}

bool control_handleJson(const String& requestBody, String& responseJson) {
  char command[32];
  if (!extractString(requestBody, "cmd", command, sizeof(command))) {
    makeResponse(responseJson, false, "missing cmd");
    return false;
  }

  normalizeToken(command);

  if (strcmp(command, "SET_GROW_MODE") == 0) {
    char mode[20];
    if (!extractString(requestBody, "mode", mode, sizeof(mode))) {
      makeResponse(responseJson, false, "missing mode");
      return false;
    }

    normalizeToken(mode);

    if (!setGrowModeByName(mode)) {
      makeResponse(responseJson, false, "invalid grow mode");
      return false;
    }

    makeResponse(responseJson, true, "grow mode updated");
    return true;
  }

  if (strcmp(command, "SET_LIGHT_MODE") == 0) {
    char mode[20];
    if (!extractString(requestBody, "mode", mode, sizeof(mode))) {
      makeResponse(responseJson, false, "missing mode");
      return false;
    }

    normalizeToken(mode);

    if (!setLightModeByName(mode)) {
      makeResponse(responseJson, false, "invalid light mode");
      return false;
    }

    makeResponse(responseJson, true, "light mode updated");
    return true;
  }

  if (strcmp(command, "SET_FAN_AUTO") == 0) {
    int fan = 0;
    extractInt(requestBody, "fan", fan);
    if (!fan_setAuto(fan)) {
      makeResponse(responseJson, false, "invalid fan");
      return false;
    }
    if (fan == 0 || fan == 1) ui_setFanAuto();
    makeResponse(responseJson, true, "fan auto enabled");
    return true;
  }

  if (strcmp(command, "SET_FAN_MANUAL") == 0) {
    int percent = 0;
    if (!extractInt(requestBody, "percent", percent)) {
      makeResponse(responseJson, false, "missing percent");
      return false;
    }

    if (percent < 0 || percent > 100) {
      makeResponse(responseJson, false, "invalid percent");
      return false;
    }

    int fan = 0;
    extractInt(requestBody, "fan", fan);
    if (!fan_setManual(fan, percent)) {
      makeResponse(responseJson, false, "invalid fan");
      return false;
    }
    if (fan == 0 || fan == 1) ui_setFanManual(percent);
    makeResponse(responseJson, true, "fan manual updated");
    return true;
  }

  if (strcmp(command, "SET_REST_MODE") == 0 || strcmp(command, "SET_REST") == 0 || strcmp(command, "REST_MODE") == 0) {
    bool enabled = false;
    if (!extractBool(requestBody, "enabled", enabled) && !extractBool(requestBody, "state", enabled)) {
      makeResponse(responseJson, false, "missing enabled");
      return false;
    }

    if (!restMode_setEnabled(enabled, true)) {
      makeResponse(responseJson, false, "rest mode settings unavailable");
      return false;
    }

    if (enabled) {
      light_loop();
      fan_forceOff();
      pumpScheduler_manualStop();
    }

    makeResponse(responseJson, true, enabled ? "rest mode enabled" : "rest mode disabled");
    return true;
  }

  if (strcmp(command, "SET_OUTPUT") == 0) {
    char output[20];
    bool on = false;
    if (!extractString(requestBody, "output", output, sizeof(output))) {
      makeResponse(responseJson, false, "missing output");
      return false;
    }
    normalizeToken(output);

    if (!extractBool(requestBody, "state", on) && !extractBool(requestBody, "on", on)) {
      makeResponse(responseJson, false, "missing state");
      return false;
    }

    if (!outputs_setByName(output, on)) {
      makeResponse(responseJson, false, "invalid output");
      return false;
    }

    makeResponse(responseJson, true, "output updated");
    return true;
  }

  if (strcmp(command, "SET_LIGHT_SCHEDULE") == 0) {
    int onHour = 0;
    int offHour = 0;
    if (!extractInt(requestBody, "on_hour", onHour) || !extractInt(requestBody, "off_hour", offHour)) {
      makeResponse(responseJson, false, "missing light schedule");
      return false;
    }

    if (!light_setSchedule(onHour, offHour)) {
      makeResponse(responseJson, false, "invalid light schedule");
      return false;
    }

    makeResponse(responseJson, true, "light schedule updated");
    return true;
  }

  if (strcmp(command, "SET_CLIMATE_TARGETS") == 0) {
    float dayTemp = climate_getDayTemp();
    float nightTemp = climate_getNightTemp();
    float tempValue = 0.0;
    int dayHum = static_cast<int>(climate_getDayHum());
    int nightHum = static_cast<int>(climate_getNightHum());
    int humValue = 0;

    if (extractFloat(requestBody, "day_temp_c", tempValue)) dayTemp = tempValue;
    if (extractFloat(requestBody, "night_temp_c", tempValue)) nightTemp = tempValue;
    if (extractInt(requestBody, "day_hum_pct", humValue)) dayHum = humValue;
    if (extractInt(requestBody, "night_hum_pct", humValue)) nightHum = humValue;

    if (!climate_setTargets(dayTemp, nightTemp, dayHum, nightHum)) {
      makeResponse(responseJson, false, "climate settings unavailable");
      return false;
    }

    makeResponse(responseJson, true, "climate targets updated");
    return true;
  }

  if (strcmp(command, "PUMP_TEST") == 0) {
    char action[16];
    if (!extractString(requestBody, "action", action, sizeof(action))) {
      makeResponse(responseJson, false, "missing action");
      return false;
    }

    normalizeToken(action);

    if (strcmp(action, "START") == 0) {
      if (restMode_isEnabled()) {
        makeResponse(responseJson, false, "rest mode active");
        return false;
      }
      pumpScheduler_manualStart();
      makeResponse(responseJson, true, "pump test started");
      return true;
    }

    if (strcmp(action, "STOP") == 0) {
      pumpScheduler_manualStop();
      makeResponse(responseJson, true, "pump test stopped");
      return true;
    }

    makeResponse(responseJson, false, "invalid pump action");
    return false;
  }

  if (strcmp(command, "SET_PUMP_SCHEDULE") == 0) {
    int hour = 0;
    int minute = 0;
    if (!extractInt(requestBody, "hour", hour) || !extractInt(requestBody, "minute", minute)) {
      makeResponse(responseJson, false, "missing pump schedule");
      return false;
    }

    if (!pumpScheduler_setSchedule(hour, minute)) {
      makeResponse(responseJson, false, "invalid pump schedule");
      return false;
    }

    makeResponse(responseJson, true, "pump schedule updated");
    return true;
  }

  if (strcmp(command, "SET_PUMP_DURATION") == 0) {
    int durationSeconds = 0;
    if (!extractInt(requestBody, "duration_s", durationSeconds)) {
      makeResponse(responseJson, false, "missing pump duration");
      return false;
    }

    if (!pumpScheduler_setRunDurationSeconds(durationSeconds)) {
      makeResponse(responseJson, false, "invalid pump duration");
      return false;
    }

    makeResponse(responseJson, true, "pump duration updated");
    return true;
  }

  if (strcmp(command, "SET_CONFIG") == 0) {
    return runtimeConfig_applyJson(requestBody, responseJson);
  }

  if (strcmp(command, "SET_LOCAL_PRESET") == 0 || strcmp(command, "SAVE_LOCAL_PRESET") == 0) {
    int slot = -1;
    String payloadHex;
    if (!extractInt(requestBody, "slot", slot)) {
      makeResponse(responseJson, false, "missing slot");
      return false;
    }
    if (!extractStringValue(requestBody, "payload_hex", payloadHex)) {
      makeResponse(responseJson, false, "missing payload");
      return false;
    }
    if (!presetStore_saveHexPayload(slot, payloadHex)) {
      makeResponse(responseJson, false, "invalid local preset");
      return false;
    }
    makeResponse(responseJson, true, "local preset saved");
    return true;
  }

  if (strcmp(command, "CLEAR_LOCAL_PRESET") == 0 || strcmp(command, "DELETE_LOCAL_PRESET") == 0) {
    int slot = -1;
    if (!extractInt(requestBody, "slot", slot)) {
      makeResponse(responseJson, false, "missing slot");
      return false;
    }
    if (!presetStore_clearSlot(slot)) {
      makeResponse(responseJson, false, "invalid local preset slot");
      return false;
    }
    makeResponse(responseJson, true, "local preset cleared");
    return true;
  }

  if (strcmp(command, "SET_ACTIVE_LOCAL_PRESET") == 0) {
    int slot = -1;
    if (!extractInt(requestBody, "slot", slot)) {
      makeResponse(responseJson, false, "missing slot");
      return false;
    }
    if (!presetStore_setActiveSlot(slot)) {
      makeResponse(responseJson, false, "local preset slot is empty");
      return false;
    }
    makeResponse(responseJson, true, "active local preset updated");
    return true;
  }

  if (strcmp(command, "START_LOCAL_RUN") == 0) {
    int slot = -1;
    unsigned long startAtS = 0;
    unsigned long long startAtMs = 0;
    unsigned long revisionValue = 0;
    char runId[40] = "";
    if (!extractInt(requestBody, "slot", slot)) {
      makeResponse(responseJson, false, "missing slot");
      return false;
    }
    if (extractUnsignedLongLong(requestBody, "start_at_ms", startAtMs) && startAtMs > 0) {
      startAtS = static_cast<unsigned long>(startAtMs / 1000ULL);
    } else {
      extractUnsignedLong(requestBody, "start_at_s", startAtS);
    }
    extractUnsignedLong(requestBody, "revision", revisionValue);
    extractString(requestBody, "run_id", runId, sizeof(runId));
    if (!localRun_start(slot, startAtS, runId, static_cast<uint32_t>(revisionValue))) {
      makeResponse(responseJson, false, "local run start failed");
      return false;
    }
    makeResponse(responseJson, true, "local run started");
    return true;
  }

  if (strcmp(command, "STOP_LOCAL_RUN") == 0) {
    if (!localRun_stop()) {
      makeResponse(responseJson, false, "local run stop failed");
      return false;
    }
    makeResponse(responseJson, true, "local run stopped");
    return true;
  }

  if (strcmp(command, "PAUSE_LOCAL_RUN") == 0) {
    if (!localRun_pause(true)) {
      makeResponse(responseJson, false, "local run pause failed");
      return false;
    }
    makeResponse(responseJson, true, "local run paused");
    return true;
  }

  if (strcmp(command, "RESUME_LOCAL_RUN") == 0) {
    if (!localRun_pause(false)) {
      makeResponse(responseJson, false, "local run resume failed");
      return false;
    }
    makeResponse(responseJson, true, "local run resumed");
    return true;
  }

  if (strcmp(command, "SET_RTC_CONFIG") == 0 || strcmp(command, "SET_RTC") == 0) {
    bool enabled = false;
    if (!extractBool(requestBody, "enabled", enabled)) {
      makeResponse(responseJson, false, "missing enabled");
      return false;
    }

    rtc_setEnabled(enabled);
    makeResponse(responseJson, true, enabled ? "rtc enabled" : "rtc disabled");
    return true;
  }

  makeResponse(responseJson, false, "unknown cmd");
  return false;
}
