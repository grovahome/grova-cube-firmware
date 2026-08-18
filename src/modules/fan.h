#pragma once

void fan_preinit();
void fan_begin();
void fan_loop();
void fan_forceOff();
bool fan_applyPolicyConfig(int fan);

int getFanPercent();
int getFanTargetPercent();
int getFanRPM();
int getFan2RPM();
int getFan2Percent();
int getFan2TargetPercent();
int fan_getTemperatureDemandPercent(int fan);
int fan_getHumidityDemandPercent(int fan);
int fan_getIntervalDemandPercent(int fan);
int fan_getScheduleDemandPercent(int fan);
int fan_getRuleDemandCount(int fan);
int fan_getRuleDemandPercent(int fan, int ruleIndex);
const char* fan_getRuleDemandId(int fan, int ruleIndex);
const char* fan_getWinningRuleId(int fan);
const char* fan_getWinningProducerName(int fan);
const char* fan_getDecisionPriorityName(int fan);
bool fan_isEnabled(int fan);
bool fan_isManual(int fan);
int fan_getManualPercent(int fan);
bool fan_setAuto(int fan, bool persist = true);
bool fan_setManual(int fan, int percent, bool persist = true);
bool fan_hasTachoFault();
const char* fan_getTachoStatusName();
const char* fan_getReasonName();
const char* fan_getModeName(int fan);
const char* fan_getReasonName(int fan);
bool fan_getTachoFault(int fan);
