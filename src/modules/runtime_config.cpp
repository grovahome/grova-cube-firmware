#include <Arduino.h>
#include <Preferences.h>
#include <ctype.h>
#include <math.h>
#include "config.h"
#include "modules/runtime_config.h"

static constexpr int CONFIG_VERSION = 1;
static constexpr int TEMP_MIN_X10_DEFAULT = 180;
static constexpr int TEMP_MAX_X10_DEFAULT = 280;
static constexpr int HUM_MIN_DEFAULT = 45;
static constexpr int HUM_MAX_DEFAULT = 85;
static constexpr int TEMP_MIN_X10_LIMIT = 100;
static constexpr int TEMP_MAX_X10_LIMIT = 400;
static constexpr int HUM_MIN_LIMIT = 20;
static constexpr int HUM_MAX_LIMIT = 95;

static const int DEFAULT_TEMP_OVER_X10[FAN_CURVE_POINT_COUNT] = {0, 8, 15, 25, 40};
static const int DEFAULT_HUM_OVER[FAN_CURVE_POINT_COUNT] = {0, 5, 10, 15, 25};
static const int DEFAULT_FAN_PCT[FAN_CURVE_POINT_COUNT] = {
  FAN_IDLE_PERCENT,
  FAN_MIN_ACTIVE_PERCENT,
  50,
  75,
  FAN_MAX_PERCENT
};

static Preferences settings;
static bool settingsReady = false;
static int tempMinX10 = TEMP_MIN_X10_DEFAULT;
static int tempMaxX10 = TEMP_MAX_X10_DEFAULT;
static int humMin = HUM_MIN_DEFAULT;
static int humMax = HUM_MAX_DEFAULT;
static int tempOverX10[FAN_CURVE_POINT_COUNT];
static int humOver[FAN_CURVE_POINT_COUNT];
static int fanPct[FAN_CURVE_POINT_COUNT];

static int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static float x10ToFloat(int value) {
  return value / 10.0;
}

static void setDefaults() {
  tempMinX10 = TEMP_MIN_X10_DEFAULT;
  tempMaxX10 = TEMP_MAX_X10_DEFAULT;
  humMin = HUM_MIN_DEFAULT;
  humMax = HUM_MAX_DEFAULT;

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    tempOverX10[i] = DEFAULT_TEMP_OVER_X10[i];
    humOver[i] = DEFAULT_HUM_OVER[i];
    fanPct[i] = DEFAULT_FAN_PCT[i];
  }
}

static void normalizeConfig() {
  tempMinX10 = clampInt(tempMinX10, TEMP_MIN_X10_LIMIT, TEMP_MAX_X10_LIMIT);
  tempMaxX10 = clampInt(tempMaxX10, TEMP_MIN_X10_LIMIT, TEMP_MAX_X10_LIMIT);
  if (tempMaxX10 <= tempMinX10) tempMaxX10 = tempMinX10 + 10;

  humMin = clampInt(humMin, HUM_MIN_LIMIT, HUM_MAX_LIMIT);
  humMax = clampInt(humMax, HUM_MIN_LIMIT, HUM_MAX_LIMIT);
  if (humMax <= humMin) humMax = humMin + 1;

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    tempOverX10[i] = clampInt(tempOverX10[i], 0, 100);
    humOver[i] = clampInt(humOver[i], 0, 60);
    fanPct[i] = clampInt(fanPct[i], 0, 100);

    if (i > 0) {
      if (tempOverX10[i] < tempOverX10[i - 1]) tempOverX10[i] = tempOverX10[i - 1];
      if (humOver[i] < humOver[i - 1]) humOver[i] = humOver[i - 1];
      if (fanPct[i] < fanPct[i - 1]) fanPct[i] = fanPct[i - 1];
    }
  }
}

static void saveConfig() {
  if (!settingsReady) return;

  settings.putInt("ver", CONFIG_VERSION);
  settings.putInt("tmin", tempMinX10);
  settings.putInt("tmax", tempMaxX10);
  settings.putInt("hmin", humMin);
  settings.putInt("hmax", humMax);

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    char key[8];

    snprintf(key, sizeof(key), "to%d", i);
    settings.putInt(key, tempOverX10[i]);

    snprintf(key, sizeof(key), "ho%d", i);
    settings.putInt(key, humOver[i]);

    snprintf(key, sizeof(key), "fp%d", i);
    settings.putInt(key, fanPct[i]);
  }

  Serial.println("Runtime config saved");
}

static bool extractNumberAfter(const String& body, int startPos, float& value) {
  int pos = startPos;
  while (pos < static_cast<int>(body.length()) && isspace(body[pos])) pos++;

  int end = pos;
  while (end < static_cast<int>(body.length()) &&
         (isdigit(body[end]) || body[end] == '-' || body[end] == '.')) {
    end++;
  }

  if (end == pos) return false;

  value = body.substring(pos, end).toFloat();
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

  return extractNumberAfter(body, colonPos + 1, value);
}

static bool extractIndexedFloat(const String& body, const char* prefix, int index, float& value) {
  String key = prefix;
  key += index;
  return extractFloat(body, key.c_str(), value);
}

static bool extractNthFloat(const String& body, const char* key, int index, float& value) {
  String needle = "\"";
  needle += key;
  needle += "\"";

  int searchFrom = 0;

  for (int i = 0; i <= index; i++) {
    int keyPos = body.indexOf(needle, searchFrom);
    if (keyPos < 0) return false;

    int colonPos = body.indexOf(':', keyPos + needle.length());
    if (colonPos < 0) return false;

    if (i == index) return extractNumberAfter(body, colonPos + 1, value);
    searchFrom = colonPos + 1;
  }

  return false;
}

static int evaluateCurve(float need, bool tempCurve) {
  int result = fanPct[0];

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    float threshold = tempCurve ? x10ToFloat(tempOverX10[i]) : static_cast<float>(humOver[i]);
    if (need >= threshold) result = fanPct[i];
  }

  return result;
}

void runtimeConfig_begin() {
  setDefaults();
  settingsReady = settings.begin("vgrow-runcfg", false);

  if (!settingsReady) {
    Serial.println("Runtime config unavailable");
    return;
  }

  tempMinX10 = settings.getInt("tmin", TEMP_MIN_X10_DEFAULT);
  tempMaxX10 = settings.getInt("tmax", TEMP_MAX_X10_DEFAULT);
  humMin = settings.getInt("hmin", HUM_MIN_DEFAULT);
  humMax = settings.getInt("hmax", HUM_MAX_DEFAULT);

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    char key[8];

    snprintf(key, sizeof(key), "to%d", i);
    tempOverX10[i] = settings.getInt(key, DEFAULT_TEMP_OVER_X10[i]);

    snprintf(key, sizeof(key), "ho%d", i);
    humOver[i] = settings.getInt(key, DEFAULT_HUM_OVER[i]);

    snprintf(key, sizeof(key), "fp%d", i);
    fanPct[i] = settings.getInt(key, DEFAULT_FAN_PCT[i]);
  }

  normalizeConfig();
  Serial.println("Runtime config loaded");
}

bool runtimeConfig_settingsReady() {
  return settingsReady;
}

float runtimeConfig_getTempMinC() {
  return x10ToFloat(tempMinX10);
}

float runtimeConfig_getTempMaxC() {
  return x10ToFloat(tempMaxX10);
}

int runtimeConfig_getHumMinPct() {
  return humMin;
}

int runtimeConfig_getHumMaxPct() {
  return humMax;
}

FanCurvePoint runtimeConfig_getFanCurvePoint(int index) {
  index = clampInt(index, 0, FAN_CURVE_POINT_COUNT - 1);
  return {x10ToFloat(tempOverX10[index]), humOver[index], fanPct[index]};
}

int runtimeConfig_evaluateFanPercent(float tempOverC, float humOverPct) {
  if (tempOverC < 0) tempOverC = 0;
  if (humOverPct < 0) humOverPct = 0;

  int fanFromTemp = evaluateCurve(tempOverC, true);
  int fanFromHum = evaluateCurve(humOverPct, false);
  return max(fanFromTemp, fanFromHum);
}

void runtimeConfig_appendJson(String& json) {
  json += "\"config\":{";
  json += "\"version\":";
  json += CONFIG_VERSION;
  json += ",\"temp_min_c\":";
  json += String(runtimeConfig_getTempMinC(), 1);
  json += ",\"temp_max_c\":";
  json += String(runtimeConfig_getTempMaxC(), 1);
  json += ",\"hum_min_pct\":";
  json += humMin;
  json += ",\"hum_max_pct\":";
  json += humMax;
  json += ",\"fan_curve\":[";

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    if (i > 0) json += ",";
    json += "{\"temp_over_c\":";
    json += String(x10ToFloat(tempOverX10[i]), 1);
    json += ",\"hum_over_pct\":";
    json += humOver[i];
    json += ",\"fan_pct\":";
    json += fanPct[i];
    json += "}";
  }

  json += "]}";
}

bool runtimeConfig_applyJson(const String& body, String& responseJson) {
  float value = 0;

  if (extractFloat(body, "temp_min_c", value)) tempMinX10 = lroundf(value * 10.0);
  if (extractFloat(body, "temp_max_c", value)) tempMaxX10 = lroundf(value * 10.0);
  if (extractFloat(body, "hum_min_pct", value)) humMin = lroundf(value);
  if (extractFloat(body, "hum_max_pct", value)) humMax = lroundf(value);

  for (int i = 0; i < FAN_CURVE_POINT_COUNT; i++) {
    if (extractIndexedFloat(body, "temp_over_c_", i, value) ||
        extractNthFloat(body, "temp_over_c", i, value)) {
      tempOverX10[i] = lroundf(value * 10.0);
    }

    if (extractIndexedFloat(body, "hum_over_pct_", i, value) ||
        extractNthFloat(body, "hum_over_pct", i, value)) {
      humOver[i] = lroundf(value);
    }

    if (extractIndexedFloat(body, "fan_pct_", i, value) ||
        extractNthFloat(body, "fan_pct", i, value)) {
      fanPct[i] = lroundf(value);
    }
  }

  normalizeConfig();
  saveConfig();

  responseJson = "{\"ok\":true,\"message\":\"runtime config saved\"}";
  return true;
}
