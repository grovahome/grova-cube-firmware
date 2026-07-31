#pragma once

#include <Arduino.h>

enum SensorStatus {
  SENSOR_WAITING,
  SENSOR_OK,
  SENSOR_READ_FAIL,
  SENSOR_OUT_OF_RANGE
};

void sensors_begin();
void sensors_loop();

float getTemp();
float getHum();
float sensors_getPressureHpa();
float sensors_getBoschTemp();
float sensors_getCo2Ppm();
float sensors_getLux();
float sensors_getUvIndex();
const char* sensors_getSourceName();
const char* sensors_getPressureSourceName();
const char* sensors_getCo2SourceName();
const char* sensors_getLuxSourceName();
const char* sensors_getUvSourceName();
bool sensors_hasPressure();
void sensors_appendSourcesJson(String& json);

SensorStatus sensors_getStatus();
const char* sensors_getStatusName();
bool sensors_hasFault();
unsigned long sensors_getFailCount();
unsigned int sensors_getConsecutiveFailCount();
