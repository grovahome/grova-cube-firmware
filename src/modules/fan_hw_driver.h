#pragma once

void fanHw_preinit();
void fanHw_begin();
void fanHw_update();

bool fanHw_isEnabled(int fan);
bool fanHw_isTachoEnabled(int fan);
void fanHw_writePwm(int fan, int pwm);
void fanHw_emergencyStop(int fan);
int fanHw_getPwm(int fan);
int fanHw_getRpm(int fan);

