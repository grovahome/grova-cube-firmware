#pragma once

void restMode_begin();
bool restMode_isEnabled();
bool restMode_settingsReady();
bool restMode_setEnabled(bool enabled, bool saveNow);
const char* restMode_getName();
const char* restMode_getReasonName();
