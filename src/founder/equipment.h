#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace founderEquipment {
void begin();
bool settingsReady();
bool hasAssignments();
bool fanEnabled(unsigned fan);
bool configure(JsonObjectConst request, String& message);
bool operate(JsonObjectConst request, String& message);
void stopAll();
void appendJson(String& json);
}  // namespace founderEquipment
