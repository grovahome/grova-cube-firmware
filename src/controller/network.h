#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace founderNetwork {
void begin();
void loop();
bool configure(JsonObjectConst object);
void appendConfig(JsonObject object);
bool connected();
}  // namespace founderNetwork
