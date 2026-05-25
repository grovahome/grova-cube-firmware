#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>

#include "config.h"
#include "firmware_info.h"
#include "modules/alarms.h"
#include "modules/climate.h"
#include "modules/control.h"
#include "modules/fan.h"
#include "modules/grow_mode.h"
#include "modules/light.h"
#include "modules/mqtt_client.h"
#include "modules/pump_scheduler.h"
#include "modules/runtime_config.h"
#include "modules/sensors.h"
#include "modules/stability.h"
#include "modules/time_sync.h"
#include "modules/ui.h"
#include "modules/wifi_ota.h"

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastTelemetryPublish = 0;
static char telemetryTopic[96];
static char commandTopic[96];
static char ackTopic[96];

static bool mqttEnabled() {
  return strlen(MQTT_HOST) > 0 && strlen(MQTT_CUBE_ID) > 0;
}

static void appendJsonString(String& json, const char* key, const char* value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  json += value;
  json += "\"";
  if (comma) json += ",";
}

static void appendJsonBool(String& json, const char* key, bool value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (comma) json += ",";
}

static void appendJsonInt(String& json, const char* key, long value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value;
  if (comma) json += ",";
}

static void appendJsonFloat(String& json, const char* key, float value, int decimals, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += String(value, decimals);
  if (comma) json += ",";
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

static String buildTelemetryJson() {
  float temp = getTemp();
  float hum = getHum();
  bool settingsOk =
    ui_settingsReady() &&
    growMode_settingsReady() &&
    climate_settingsReady() &&
    light_settingsReady() &&
    pumpScheduler_settingsReady() &&
    runtimeConfig_settingsReady();

  String json;
  json.reserve(1700);
  json += "{";
  appendJsonString(json, "cube_id", MQTT_CUBE_ID);
  appendJsonFloat(json, "temp_c", temp, 1);
  appendJsonFloat(json, "hum_pct", hum, 1);
  appendJsonFloat(json, "target_temp_c", getTargetTemp(), 1);
  appendJsonFloat(json, "target_hum_pct", getTargetHum(), 0);
  appendJsonString(json, "warning", alarms_getPrimaryWarning(temp, hum));
  appendJsonBool(json, "healthy", isSystemHealthy());
  appendJsonInt(json, "uptime_s", millis() / 1000UL);
  appendJsonInt(json, "free_heap", ESP.getFreeHeap());
  appendJsonBool(json, "settings_ok", settingsOk);
  appendJsonString(json, "firmware_name", GROVA_FIRMWARE_NAME);
  appendJsonString(json, "firmware_version", GROVA_FIRMWARE_VERSION);
  appendJsonString(json, "firmware_build_date", GROVA_BUILD_DATE);
  appendJsonString(json, "firmware_build_time", GROVA_BUILD_TIME);
  appendJsonString(json, "firmware_build_env", GROVA_BUILD_ENV);
  appendJsonBool(json, "wifi_connected", wifiOTA_isConnected());
  appendJsonBool(json, "time_synced", isTimeSynced());
  appendJsonInt(json, "hour", getHour());
  appendJsonInt(json, "minute", getMinute());
  runtimeConfig_appendJson(json);
  json += ",";

  json += "\"climate_targets\":{";
  appendJsonFloat(json, "day_temp_c", climate_getDayTemp(), 1);
  appendJsonFloat(json, "night_temp_c", climate_getNightTemp(), 1);
  appendJsonFloat(json, "day_hum_pct", climate_getDayHum(), 0);
  appendJsonFloat(json, "night_hum_pct", climate_getNightHum(), 0, false);
  json += "},";

  json += "\"grow\":{";
  appendJsonString(json, "mode", growMode_getName());
  appendJsonString(json, "effect", growMode_getEffectName());
  appendJsonBool(json, "germination", growMode_isGermination());
  appendJsonBool(json, "harvest", growMode_isHarvest(), false);
  json += "},";

  json += "\"fan\":{";
  appendJsonString(json, "mode", ui_getFanModeName());
  appendJsonInt(json, "current_pct", getFanPercent());
  appendJsonInt(json, "target_pct", getFanTargetPercent());
  appendJsonString(json, "reason", fan_getReasonName());
  appendJsonString(json, "tacho", fan_getTachoStatusName());
  appendJsonInt(json, "rpm", getFanRPM(), false);
  json += "},";

  json += "\"light\":{";
  appendJsonBool(json, "on", isLightOn());
  appendJsonString(json, "mode", ui_getLightModeName());
  appendJsonString(json, "reason", light_getReasonName());
  appendJsonInt(json, "on_hour", light_getOnHour());
  appendJsonInt(json, "off_hour", light_getOffHour(), false);
  json += "},";

  json += "\"pump\":{";
  appendJsonString(json, "mode", pumpScheduler_getModeName());
  appendJsonString(json, "reason", pumpScheduler_getReasonName());
  appendJsonBool(json, "running", pumpScheduler_isRunning());
  appendJsonInt(json, "remaining_s", pumpScheduler_getRemainingSeconds());
  appendJsonInt(json, "hour", pumpScheduler_getRunHour());
  appendJsonInt(json, "minute", pumpScheduler_getRunMinute());
  appendJsonInt(json, "duration_s", pumpScheduler_getRunDurationSeconds());
  appendJsonInt(json, "max_duration_s", pumpScheduler_getMaxRunDurationSeconds());
  appendJsonInt(json, "runs_today", pumpScheduler_getRunsToday());
  appendJsonInt(json, "max_runs_per_day", pumpScheduler_getMaxRunsPerDay());
  appendJsonInt(json, "min_interval_h", pumpScheduler_getMinIntervalHours());
  appendJsonBool(json, "startup_locked", pumpScheduler_isStartupLocked());
  appendJsonBool(json, "today_done", pumpScheduler_getRunsToday() >= pumpScheduler_getMaxRunsPerDay(), false);
  json += "},";

  json += "\"sensor\":{";
  appendJsonString(json, "status", sensors_getStatusName());
  appendJsonInt(json, "fail_count", sensors_getFailCount());
  appendJsonBool(json, "fault", sensors_hasFault(), false);
  json += "}";

  json += "}";
  return json;
}

static String buildAckJson(const char* cmdId, bool ok, const String& responseJson) {
  String json;
  json.reserve(responseJson.length() + 96);
  json += "{";
  appendJsonString(json, "cube_id", MQTT_CUBE_ID);
  if (strlen(cmdId) > 0) {
    appendJsonString(json, "cmd_id", cmdId);
  }
  appendJsonBool(json, "ok", ok);
  json += "\"response\":";
  json += responseJson;
  json += "}";
  return json;
}

static void publishTelemetry() {
  String payload = buildTelemetryJson();
  if (!mqtt.publish(telemetryTopic, payload.c_str())) {
    Serial.println("MQTT telemetry publish failed");
  }
}

static void handleCommand(char* topic, byte* payload, unsigned int length) {
  (void)topic;

  String body;
  body.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    body += static_cast<char>(payload[i]);
  }

  char cmdId[48];
  extractString(body, "cmd_id", cmdId, sizeof(cmdId));

  String response;
  bool ok = control_handleJson(body, response);
  String ack = buildAckJson(cmdId, ok, response);
  mqtt.publish(ackTopic, ack.c_str());
  publishTelemetry();
}

static bool connectMqtt() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return false;

  String clientId = "grova-";
  clientId += MQTT_CUBE_ID;
  clientId += "-";
  clientId += String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);

  bool connected;
  if (strlen(MQTT_USER) > 0) {
    connected = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  } else {
    connected = mqtt.connect(clientId.c_str());
  }

  if (!connected) {
    Serial.print("MQTT connect failed, state ");
    Serial.println(mqtt.state());
    return false;
  }

  mqtt.subscribe(commandTopic);
  Serial.println("MQTT connected");
  publishTelemetry();
  return true;
}

void mqttClient_begin() {
  if (!mqttEnabled()) {
    Serial.println("MQTT disabled");
    return;
  }

  snprintf(telemetryTopic, sizeof(telemetryTopic), "grova/v1/cubes/%s/telemetry", MQTT_CUBE_ID);
  snprintf(commandTopic, sizeof(commandTopic), "grova/v1/cubes/%s/command", MQTT_CUBE_ID);
  snprintf(ackTopic, sizeof(ackTopic), "grova/v1/cubes/%s/ack", MQTT_CUBE_ID);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(handleCommand);
  mqtt.setBufferSize(2200);
  Serial.println("MQTT ready");
}

void mqttClient_loop() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected()) {
    if (millis() - lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL_MS) {
      lastReconnectAttempt = millis();
      connectMqtt();
    }
    return;
  }

  mqtt.loop();

  if (millis() - lastTelemetryPublish >= MQTT_TELEMETRY_INTERVAL_MS) {
    lastTelemetryPublish = millis();
    publishTelemetry();
  }
}
