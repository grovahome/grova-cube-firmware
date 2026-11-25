#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace founderEquipment {
enum class LocalMode { Manual, Scheduled, Sensor };
constexpr unsigned LocalDeviceCount = 7;
struct LocalDevice {
  const char* name;
  LocalMode mode;
  bool timed;
  bool variable;
  unsigned percent;
  unsigned overrideSeconds;
};
struct NextAction {
  const char* name = nullptr;
  unsigned minute = 0, minutesUntil = 1441, percent = 0;
  bool pulse = false, variable = false;
};
uint32_t localRevision();
bool localDevice(unsigned index, LocalDevice& device);
bool setLocalLevel(unsigned index, unsigned percent);
bool cancelLocalOverride(unsigned index);
bool stopLocalTimed(unsigned index);
NextAction nextAction();
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
