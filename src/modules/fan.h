#pragma once

void fan_preinit();
void fan_begin();
void fan_loop(float temp, float hum);

int getFanPercent();
int getFanTargetPercent();
int getFanRPM();
int getFan2RPM();
bool fan_hasTachoFault();
const char* fan_getTachoStatusName();
const char* fan_getReasonName();
