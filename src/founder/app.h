#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace founderApp {
void bootSafetyBegin();
void settingsBegin();
void loop();
String status();
bool command(const String& body, String& response, bool remote = false);
const char* deviceId();
bool setRest(bool enabled);
bool hasFault();
}  // namespace founderApp
