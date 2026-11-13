#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace founderEquipment {
void begin();
bool settingsReady();
bool hasAssignments();
bool fanEnabled(unsigned fan);
bool setFanManual(unsigned fan, unsigned percent);
bool fanControlled(unsigned fan);
bool configure(JsonObjectConst request, String& message);
bool operate(JsonObjectConst request, String& message);
void stopAll();
void loop();
bool hasPlans();
bool planEnabled(const char* id);
void appendJson(String& json);
}  // namespace founderEquipment
