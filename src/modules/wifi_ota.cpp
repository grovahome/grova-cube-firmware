#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <string.h>
#include "config.h"

static unsigned long lastReconnect = 0;
static String ipText = "-";

void wifiOTA_begin() {

  if (strlen(WIFI_SSID) == 0) {
    Serial.println("WiFi nicht konfiguriert, OTA deaktiviert");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi verbindet");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK");
    Serial.println(WiFi.localIP());
    ipText = WiFi.localIP().toString();
  } else {
    Serial.println("\nWiFi FAIL");
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.begin();
}

void wifiOTA_loop() {
  if (strlen(WIFI_SSID) == 0) return;

  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect >= 10000) {
    lastReconnect = millis();
    ipText = "-";
    WiFi.reconnect();
  }

  if (WiFi.status() == WL_CONNECTED) {
    ipText = WiFi.localIP().toString();
  }
}

bool wifiOTA_isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

const char* wifiOTA_getIP() {
  return ipText.c_str();
}
