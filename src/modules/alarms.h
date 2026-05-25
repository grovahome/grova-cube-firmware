#pragma once

bool alarms_hasWarning(float temp, float hum);
const char* alarms_getPrimaryWarning(float temp, float hum);
