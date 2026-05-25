#pragma once

void fan_preinit();
void fan_begin();
void fan_loop(float temp, float hum);

int getFanPercent();
int getFanTargetPercent();
int getFanRPM();
bool fan_hasTachoFault();
const char* fan_getTachoStatusName();
const char* fan_getReasonName();
