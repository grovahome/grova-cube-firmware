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
#include "modules/i2c_discovery.h"
#include "modules/light.h"
#include "modules/local_run.h"
#include "modules/mqtt_client.h"
#include "modules/outputs.h"
#include "modules/preset_store.h"
#include "modules/pump_scheduler.h"
#include "modules/rest_mode.h"
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
static char availabilityTopic[96];
static char discoveryTopic[128];
static char homeAssistantStatusTopic[96];

static bool mqttEnabled() {
  return GROVA_MQTT_ENABLED && strlen(MQTT_HOST) > 0 && strlen(MQTT_CUBE_ID) > 0;
}

static void appendEscapedJsonValue(String& json, const char* value) {
  for (const char* cursor = value; *cursor; cursor++) {
    char c = *cursor;
    if (c == '"' || c == '\\') {
      json += '\\';
      json += c;
    } else if (c == '\n') {
      json += "\\n";
    } else if (c == '\r') {
      json += "\\r";
    } else if (c == '\t') {
      json += "\\t";
    } else {
      json += c;
    }
  }
}

static void appendJsonString(String& json, const char* key, const char* value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  appendEscapedJsonValue(json, value);
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
  if (isnan(value)) {
    json += "null";
  } else {
    json += String(value, decimals);
  }
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
    restMode_settingsReady() &&
    presetStore_settingsReady() &&
    localRun_settingsReady() &&
    runtimeConfig_settingsReady() &&
    time_settingsReady();

  String json;
  json.reserve(3600);
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
  appendJsonString(json, "time_source", time_getSourceName());
  appendJsonInt(json, "hour", getHour());
  appendJsonInt(json, "minute", getMinute());

  json += "\"environment\":{";
  appendJsonFloat(json, "temperature_c", temp, 1);
  appendJsonFloat(json, "humidity_pct", hum, 1);
  appendJsonFloat(json, "pressure_hpa", sensors_getPressureHpa(), 0);
  appendJsonFloat(json, "co2_ppm", sensors_getCo2Ppm(), 0);
  appendJsonFloat(json, "lux", sensors_getLux(), 0);
  appendJsonFloat(json, "uv_index", sensors_getUvIndex(), 1);
  appendJsonString(json, "temperature_source", sensors_getSourceName());
  appendJsonString(json, "humidity_source", sensors_getSourceName());
  appendJsonString(json, "pressure_source", sensors_getPressureSourceName());
  appendJsonString(json, "co2_source", sensors_getCo2SourceName());
  appendJsonString(json, "lux_source", sensors_getLuxSourceName());
  appendJsonString(json, "uv_source", sensors_getUvSourceName(), false);
  json += "},";

  runtimeConfig_appendJson(json);
  json += ",";
  presetStore_appendSummaryJson(json);
  json += ",";
  localRun_appendJson(json);
  json += ",";

  json += "\"rest_mode\":{";
  appendJsonBool(json, "enabled", restMode_isEnabled());
  appendJsonString(json, "mode", restMode_getName());
  appendJsonString(json, "reason", restMode_getReasonName(), false);
  json += "},";

  json += "\"rtc\":{";
  appendJsonBool(json, "enabled", rtc_isEnabled());
  appendJsonBool(json, "present", rtc_isPresent());
  appendJsonBool(json, "valid", rtc_hasValidTime());
  appendJsonBool(json, "used_for_boot", rtc_wasUsedForBoot());
  appendJsonBool(json, "last_read_ok", rtc_lastReadOk());
  appendJsonBool(json, "last_write_ok", rtc_lastWriteOk(), false);
  json += "},";

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
  appendJsonString(json, "mode", fan_getModeName(1));
  appendJsonInt(json, "current_pct", getFanPercent());
  appendJsonInt(json, "target_pct", getFanTargetPercent());
  appendJsonString(json, "reason", fan_getReasonName(1));
  appendJsonString(json, "tacho", fan_getTachoStatusName());
  appendJsonInt(json, "rpm", getFanRPM());
  appendJsonInt(json, "rpm2", getFan2RPM(), false);
  json += "},";

  json += "\"fan1\":{";
  appendJsonBool(json, "enabled", fan_isEnabled(1));
  appendJsonString(json, "mode", fan_getModeName(1));
  appendJsonInt(json, "current_pct", getFanPercent());
  appendJsonInt(json, "target_pct", getFanTargetPercent());
  appendJsonString(json, "reason", fan_getReasonName(1));
  appendJsonInt(json, "rpm", getFanRPM());
  appendJsonBool(json, "tacho_fault", fan_getTachoFault(1), false);
  json += "},";

  json += "\"fan2\":{";
  appendJsonBool(json, "enabled", fan_isEnabled(2));
  appendJsonString(json, "mode", fan_getModeName(2));
  appendJsonInt(json, "current_pct", getFan2Percent());
  appendJsonInt(json, "target_pct", getFan2TargetPercent());
  appendJsonString(json, "reason", fan_getReasonName(2));
  appendJsonInt(json, "rpm", getFan2RPM());
  appendJsonBool(json, "tacho_fault", fan_getTachoFault(2), false);
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
  appendJsonString(json, "safety_mode", "event_lock");
  appendJsonBool(json, "startup_locked", pumpScheduler_isStartupLocked(), false);
  json += "},";

  json += "\"outputs\":{";
  json += "\"aux_12v\":{";
  appendJsonBool(json, "available", outputs_hasAux12v());
  appendJsonBool(json, "on", outputs_isAux12vOn(), false);
  json += "},";
  json += "\"aux_5v\":{";
  appendJsonBool(json, "available", outputs_hasAux5v());
  appendJsonBool(json, "on", outputs_isAux5vOn(), false);
  json += "}";
  json += "},";

  json += "\"sensor\":{";
  appendJsonString(json, "status", sensors_getStatusName());
  appendJsonString(json, "source", sensors_getSourceName());
  appendJsonFloat(json, "pressure_hpa", sensors_getPressureHpa(), 0);
  appendJsonString(json, "pressure_source", sensors_getPressureSourceName());
  appendJsonFloat(json, "bosch_temp_c", sensors_getBoschTemp(), 1);
  appendJsonInt(json, "fail_count", sensors_getFailCount());
  appendJsonInt(json, "consecutive_fail_count", sensors_getConsecutiveFailCount());
  appendJsonBool(json, "fault", sensors_hasFault());
  sensors_appendSourcesJson(json);
  json += "}";

  json += ",";
  i2cDiscovery_appendJson(json);

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

static String haUniqueId(const char* suffix) {
  String id = "grova_";
  id += MQTT_CUBE_ID;
  id += "_";
  id += suffix;
  return id;
}

static void appendJsonStringValue(String& json, const char* value) {
  json += "\"";
  appendEscapedJsonValue(json, value);
  json += "\"";
}

static void appendAvailability(String& json) {
  appendJsonString(json, "availability_topic", availabilityTopic);
  appendJsonString(json, "payload_available", "online");
  appendJsonString(json, "payload_not_available", "offline");
}

static void appendComponentBase(String& json, const char* componentId, const char* platform, const char* uniqueSuffix, const char* name) {
  appendJsonStringValue(json, componentId);
  json += ":{";
  appendJsonString(json, "p", platform);
  appendJsonString(json, "unique_id", haUniqueId(uniqueSuffix).c_str());
  appendJsonString(json, "name", name);
  appendAvailability(json);
}

static void appendSensorComponent(String& json, const char* componentId, const char* uniqueSuffix, const char* name, const char* valueTemplate, const char* deviceClass, const char* unit, bool comma = true) {
  appendComponentBase(json, componentId, "sensor", uniqueSuffix, name);
  appendJsonString(json, "state_topic", telemetryTopic);
  appendJsonString(json, "value_template", valueTemplate);
  if (deviceClass && strlen(deviceClass) > 0) {
    appendJsonString(json, "device_class", deviceClass);
  }
  if (unit && strlen(unit) > 0) {
    appendJsonString(json, "unit_of_measurement", unit);
  }
  appendJsonString(json, "state_class", "measurement", false);
  json += "}";
  if (comma) json += ",";
}

static void appendBinarySensorComponent(String& json, const char* componentId, const char* uniqueSuffix, const char* name, const char* valueTemplate, bool comma = true) {
  appendComponentBase(json, componentId, "binary_sensor", uniqueSuffix, name);
  appendJsonString(json, "state_topic", telemetryTopic);
  appendJsonString(json, "value_template", valueTemplate);
  appendJsonString(json, "payload_on", "ON");
  appendJsonString(json, "payload_off", "OFF", false);
  json += "}";
  if (comma) json += ",";
}

static void appendButtonComponent(String& json, const char* componentId, const char* uniqueSuffix, const char* name, const char* payloadPress, bool comma = true) {
  appendComponentBase(json, componentId, "button", uniqueSuffix, name);
  appendJsonString(json, "command_topic", commandTopic);
  appendJsonString(json, "payload_press", payloadPress, false);
  json += "}";
  if (comma) json += ",";
}

static void appendNumberComponent(String& json, const char* componentId, const char* uniqueSuffix, const char* name, const char* valueTemplate, const char* commandTemplate, bool comma = true) {
  appendComponentBase(json, componentId, "number", uniqueSuffix, name);
  appendJsonString(json, "state_topic", telemetryTopic);
  appendJsonString(json, "value_template", valueTemplate);
  appendJsonString(json, "command_topic", commandTopic);
  appendJsonString(json, "command_template", commandTemplate);
  appendJsonInt(json, "min", 0);
  appendJsonInt(json, "max", 100);
  appendJsonInt(json, "step", 1);
  appendJsonString(json, "unit_of_measurement", "%");
  appendJsonString(json, "mode", "slider", false);
  json += "}";
  if (comma) json += ",";
}

static void appendSwitchComponent(String& json, const char* componentId, const char* uniqueSuffix, const char* name, const char* valueTemplate, const char* payloadOn, const char* payloadOff, bool comma = true) {
  appendComponentBase(json, componentId, "switch", uniqueSuffix, name);
  appendJsonString(json, "state_topic", telemetryTopic);
  appendJsonString(json, "value_template", valueTemplate);
  appendJsonString(json, "state_on", "ON");
  appendJsonString(json, "state_off", "OFF");
  appendJsonString(json, "command_topic", commandTopic);
  appendJsonString(json, "payload_on", payloadOn);
  appendJsonString(json, "payload_off", payloadOff, false);
  json += "}";
  if (comma) json += ",";
}

static String buildHomeAssistantDiscoveryJson() {
  String ip = WiFi.localIP().toString();
  String configurationUrl = "http://";
  configurationUrl += ip;

  String json;
  json.reserve(7600);
  json += "{";
  appendJsonString(json, "state_topic", telemetryTopic);

  json += "\"device\":{";
  json += "\"identifiers\":[";
  String identifier = "grova_";
  identifier += MQTT_CUBE_ID;
  appendJsonStringValue(json, identifier.c_str());
  json += "],";
  appendJsonString(json, "name", MQTT_DEVICE_NAME);
  appendJsonString(json, "manufacturer", GROVA_DEVICE_MANUFACTURER);
  appendJsonString(json, "model", GROVA_DEVICE_MODEL);
  appendJsonString(json, "serial_number", MQTT_CUBE_ID);
  appendJsonString(json, "hw_version", GROVA_HARDWARE_VERSION);
  appendJsonString(json, "sw_version", GROVA_FIRMWARE_VERSION);
  appendJsonString(json, "configuration_url", configurationUrl.c_str(), false);
  json += "},";

  json += "\"origin\":{";
  appendJsonString(json, "name", "GROVA Core Firmware");
  appendJsonString(json, "sw_version", GROVA_FIRMWARE_VERSION);
  appendJsonString(json, "support_url", GROVA_SUPPORT_URL, false);
  json += "},";

  json += "\"components\":{";
  appendSensorComponent(json, "temperature", "temperature", "Temperature", "{{ value_json.environment.temperature_c }}", "temperature", "\xC2\xB0" "C");
  appendSensorComponent(json, "humidity", "humidity", "Humidity", "{{ value_json.environment.humidity_pct }}", "humidity", "%");
  appendSensorComponent(json, "pressure", "pressure", "Pressure", "{{ value_json.environment.pressure_hpa }}", "pressure", "hPa");
  appendSensorComponent(json, "co2", "co2", "CO2", "{{ value_json.environment.co2_ppm }}", "carbon_dioxide", "ppm");
  appendSensorComponent(json, "lux", "lux", "Light", "{{ value_json.environment.lux }}", "illuminance", "lx");
  appendSensorComponent(json, "uv_index", "uv_index", "UV index", "{{ value_json.environment.uv_index }}", "", "");
  appendSensorComponent(json, "fan1_percent", "fan1_percent", "Fan 1 speed", "{{ value_json.fan1.current_pct }}", "", "%");
  appendSensorComponent(json, "fan1_rpm", "fan1_rpm", "Fan 1 RPM", "{{ value_json.fan1.rpm }}", "", "rpm");
#if FAN2_ENABLED
  appendSensorComponent(json, "fan2_percent", "fan2_percent", "Fan 2 speed", "{{ value_json.fan2.current_pct }}", "", "%");
  appendSensorComponent(json, "fan2_rpm", "fan2_rpm", "Fan 2 RPM", "{{ value_json.fan2.rpm }}", "", "rpm");
#endif
  appendSensorComponent(json, "pump_runs_today", "pump_runs_today", "Pump runs today", "{{ value_json.pump.runs_today }}", "", "");
  appendBinarySensorComponent(json, "healthy", "healthy", "Healthy", "{{ 'ON' if value_json.healthy else 'OFF' }}");
  appendBinarySensorComponent(json, "rest_mode_active", "rest_mode_active", "Rest mode active", "{{ 'ON' if value_json.rest_mode.enabled else 'OFF' }}");
  appendBinarySensorComponent(json, "pump_running", "pump_running", "Pump running", "{{ 'ON' if value_json.pump.running else 'OFF' }}");
#if PIN_AUX_12V >= 0
  appendBinarySensorComponent(json, "aux_12v_on", "aux_12v_on", "12V output on", "{{ 'ON' if value_json.outputs.aux_12v.on else 'OFF' }}");
#endif
#if PIN_AUX_5V >= 0
  appendBinarySensorComponent(json, "aux_5v_on", "aux_5v_on", "5V output on", "{{ 'ON' if value_json.outputs.aux_5v.on else 'OFF' }}");
#endif

  appendComponentBase(json, "light", "light", "light", "Light");
  appendJsonString(json, "state_topic", telemetryTopic);
  appendJsonString(json, "state_value_template", "{{ 'ON' if value_json.light.on else 'OFF' }}");
  appendJsonString(json, "command_topic", commandTopic);
  appendJsonString(json, "payload_on", "{\"cmd\":\"set_light_mode\",\"mode\":\"ON\"}");
  appendJsonString(json, "payload_off", "{\"cmd\":\"set_light_mode\",\"mode\":\"OFF\"}", false);
  json += "},";

  appendNumberComponent(json, "fan1_speed", "fan1_speed", "Fan 1 speed", "{{ value_json.fan1.target_pct }}", "{\"cmd\":\"set_fan_manual\",\"fan\":1,\"percent\":{{ value | int }}}");
#if FAN2_ENABLED
  appendNumberComponent(json, "fan2_speed", "fan2_speed", "Fan 2 speed", "{{ value_json.fan2.target_pct }}", "{\"cmd\":\"set_fan_manual\",\"fan\":2,\"percent\":{{ value | int }}}");
#endif

  appendButtonComponent(json, "fan1_auto", "fan1_auto", "Fan 1 auto", "{\"cmd\":\"set_fan_auto\",\"fan\":1}");
#if FAN2_ENABLED
  appendButtonComponent(json, "fan2_auto", "fan2_auto", "Fan 2 auto", "{\"cmd\":\"set_fan_auto\",\"fan\":2}");
#endif
#if PIN_AUX_12V >= 0
  appendSwitchComponent(json, "aux_12v", "aux_12v", "12V output", "{{ 'ON' if value_json.outputs.aux_12v.on else 'OFF' }}", "{\"cmd\":\"set_output\",\"output\":\"aux_12v\",\"state\":true}", "{\"cmd\":\"set_output\",\"output\":\"aux_12v\",\"state\":false}");
#endif
#if PIN_AUX_5V >= 0
  appendSwitchComponent(json, "aux_5v", "aux_5v", "5V output", "{{ 'ON' if value_json.outputs.aux_5v.on else 'OFF' }}", "{\"cmd\":\"set_output\",\"output\":\"aux_5v\",\"state\":true}", "{\"cmd\":\"set_output\",\"output\":\"aux_5v\",\"state\":false}");
#endif
  appendButtonComponent(json, "pump_test", "pump_test", "Pump test", "{\"cmd\":\"pump_test\",\"action\":\"start\"}");
  appendButtonComponent(json, "pump_stop", "pump_stop", "Stop pump", "{\"cmd\":\"pump_test\",\"action\":\"stop\"}");
  appendSwitchComponent(json, "rest_mode", "rest_mode", "Rest mode", "{{ 'ON' if value_json.rest_mode.enabled else 'OFF' }}", "{\"cmd\":\"set_rest_mode\",\"enabled\":true}", "{\"cmd\":\"set_rest_mode\",\"enabled\":false}", false);
  json += "}}";
  return json;
}

static void publishAvailability(const char* state) {
  if (!mqtt.connected()) return;
  mqtt.publish(availabilityTopic, state, true);
}

static void publishHomeAssistantDiscovery() {
  if (!GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED || !mqtt.connected()) return;
  String payload = buildHomeAssistantDiscoveryJson();
  if (!mqtt.publish(discoveryTopic, payload.c_str(), true)) {
    Serial.println("MQTT Home Assistant discovery publish failed");
    return;
  }
  Serial.println("MQTT Home Assistant discovery published");
}

static void publishTelemetry() {
  String payload = buildTelemetryJson();
  if (!mqtt.publish(telemetryTopic, payload.c_str())) {
    Serial.println("MQTT telemetry publish failed");
  }
}

static void handleCommand(char* topic, byte* payload, unsigned int length) {
  String body;
  body.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    body += static_cast<char>(payload[i]);
  }

  if (strcmp(topic, homeAssistantStatusTopic) == 0) {
    body.trim();
    if (body == "online") {
      publishHomeAssistantDiscovery();
      publishAvailability("online");
      publishTelemetry();
    }
    return;
  }

  if (strcmp(topic, commandTopic) != 0) return;

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
    connected = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, availabilityTopic, 0, true, "offline");
  } else {
    connected = mqtt.connect(clientId.c_str(), availabilityTopic, 0, true, "offline");
  }

  if (!connected) {
    Serial.print("MQTT connect failed, state ");
    Serial.println(mqtt.state());
    return false;
  }

  mqtt.subscribe(commandTopic);
  if (GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED) {
    mqtt.subscribe(homeAssistantStatusTopic);
  }
  Serial.println("MQTT connected");
  publishAvailability("online");
  publishHomeAssistantDiscovery();
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
  snprintf(availabilityTopic, sizeof(availabilityTopic), "grova/v1/cubes/%s/availability", MQTT_CUBE_ID);
  snprintf(discoveryTopic, sizeof(discoveryTopic), "%s/device/%s/config", MQTT_DISCOVERY_PREFIX, MQTT_CUBE_ID);
  snprintf(homeAssistantStatusTopic, sizeof(homeAssistantStatusTopic), "%s/status", MQTT_DISCOVERY_PREFIX);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(handleCommand);
  mqtt.setBufferSize(8192);
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
