#include "controller/network.h"

#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "controller/app.h"
#include "modules/wifi_ota.h"
namespace founderNetwork {
struct Config {
  uint32_t version = 1;
  uint16_t port = 1883;
  char host[129] = {}, user[65] = {}, password[129] = {};
};
static Config config;
static Preferences prefs;
static bool ready = false;
static WiFiClient transport;
static PubSubClient mqtt(transport);
static String base;
static uint32_t lastAttempt = 0, lastTelemetry = 0;
static bool text(const char* value, size_t max) {
  if (!value || strlen(value) > max) {
    return false;
  }
  for (const char* c = value; *c; ++c) {
    if (uint8_t(*c) < 32) {
      return false;
    }
  }
  return true;
}
void begin() {
  ready = prefs.begin("founder-mqtt", false);
  Config saved;
  if (ready && prefs.getBytesLength("config") == sizeof(saved) &&
      prefs.getBytes("config", &saved, sizeof(saved)) == sizeof(saved) && saved.version == 1 &&
      saved.port && saved.host[128] == 0 && saved.user[64] == 0 && saved.password[128] == 0 &&
      text(saved.host, 128) && text(saved.user, 64)) {
    config = saved;
  }
  base = "grova/v1/cubes/";
  base += founderApp::deviceId();
  mqtt.setServer(config.host, config.port);
  mqtt.setBufferSize(32768);
  mqtt.setSocketTimeout(1);
  transport.setTimeout(1000);
  mqtt.setCallback([](char*, byte* bytes, unsigned length) {
    if (length > 4096) {
      return;
    }
    String body;
    body.reserve(length);
    for (unsigned i = 0; i < length; ++i) {
      body += char(bytes[i]);
    }
    String response;
    founderApp::command(body, response, true);
    JsonDocument answer, request;
    deserializeJson(answer, response);
    if (!deserializeJson(request, body) && request["cmd_id"].is<const char*>()) {
      answer["cmd_id"] = request["cmd_id"];
    }
    String payload;
    serializeJson(answer, payload);
    mqtt.publish((base + "/ack").c_str(), payload.c_str(), false);
  });
}
bool configure(JsonObjectConst j) {
  if (!ready || !j["host"].is<const char*>() || !j["port"].is<unsigned>() ||
      !j["user"].is<const char*>() || !j["password"].is<const char*>()) {
    return false;
  }
  const char *host = j["host"], *user = j["user"], *password = j["password"];
  unsigned port = j["port"];
  if (!text(host, 128) || !text(user, 64) || !text(password, 128) || port < 1 || port > 65535) {
    return false;
  }
  Config next;
  next.port = port;
  strcpy(next.host, host);
  strcpy(next.user, user);
  strcpy(next.password, password);
  if (prefs.putBytes("config", &next, sizeof(next)) != sizeof(next)) {
    return false;
  }
  if (mqtt.connected()) {
    mqtt.publish((base + "/availability").c_str(), "offline", true);
  }
  mqtt.disconnect();
  config = next;
  mqtt.setServer(config.host, config.port);
  lastAttempt = millis() - 30000;
  return true;
}
void loop() {
  if (!wifiOTA_isConnected() || wifiOTA_requiresSetup() || wifiOTA_isTesting()) {
    if (mqtt.connected()) {
      mqtt.disconnect();
    }
    return;
  }
  if (!mqtt.connected()) {
    if (!*config.host || millis() - lastAttempt < 30000) {
      return;
    }
    lastAttempt = millis();
    if (mqtt.connect(founderApp::deviceId(), config.user, config.password,
                     (base + "/availability").c_str(), 1, true, "offline")) {
      mqtt.subscribe((base + "/command").c_str(), 0);
      mqtt.publish((base + "/availability").c_str(), "online", true);
      lastTelemetry = millis() - 10000;
    }
  }
  mqtt.loop();
  if (mqtt.connected() && millis() - lastTelemetry >= 10000) {
    lastTelemetry = millis();
    String payload = founderApp::status();
    mqtt.publish((base + "/telemetry").c_str(), payload.c_str(), false);
  }
}
bool connected() {
  return mqtt.connected();
}
void appendConfig(JsonObject j) {
  j["host"] = config.host;
  j["port"] = config.port;
  j["user"] = config.user;
  j["configured"] = *config.host != 0;
  j["connected"] = connected();
}
}  // namespace founderNetwork
