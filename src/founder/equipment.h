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
constexpr unsigned BackupMaxBytes = 16384;
bool backup(String& output);
bool restoreBackup(JsonObjectConst request, String& message, bool validateOnly = false);
bool configure(JsonObjectConst request, String& message, bool validateOnly = false);
bool operate(JsonObjectConst request, String& message);
void stopAll();
void loop();
void recordEvents();
bool hasPlans();
bool planEnabled(const char* id);
void appendJson(String& json);
}  // namespace founderEquipment
