#pragma once

void fan_preinit();
void fan_begin();
void fan_loop(float temp, float hum);
void fan_forceOff();

int getFanPercent();
int getFanTargetPercent();
int getFanRPM();
int getFan2RPM();
int getFan2Percent();
int getFan2TargetPercent();
int fan_getTemperatureDemandPercent(int fan);
int fan_getHumidityDemandPercent(int fan);
const char* fan_getDecisionPriorityName(int fan);
bool fan_isEnabled(int fan);
bool fan_isManual(int fan);
bool fan_setAuto(int fan);
bool fan_setManual(int fan, int percent);
bool fan_hasTachoFault();
const char* fan_getTachoStatusName();
const char* fan_getReasonName();
const char* fan_getModeName(int fan);
const char* fan_getReasonName(int fan);
bool fan_getTachoFault(int fan);
