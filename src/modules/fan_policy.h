#pragma once

#include <Arduino.h>
#include "modules/fan_control.h"

void fanPolicy_begin();
bool fanPolicy_settingsReady();
uint32_t fanPolicy_schemaVersion();
bool fanPolicy_applyJson(const String& body, String& responseJson);
int fanPolicy_extractFan(const String& body);
bool fanPolicy_setOperatingMode(int fan, FanOperatingMode mode, int manualPercent);
bool fanPolicy_reset(int fan);
