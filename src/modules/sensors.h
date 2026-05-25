#pragma once

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

SensorStatus sensors_getStatus();
const char* sensors_getStatusName();
bool sensors_hasFault();
unsigned long sensors_getFailCount();
