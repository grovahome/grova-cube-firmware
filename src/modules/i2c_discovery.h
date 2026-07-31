#pragma once

#include <Arduino.h>

struct I2cKnownDevice {
  const char* id;
  const char* name;
  const char* module;
  uint8_t address;
  bool present;
};

void i2cDiscovery_begin();
void i2cDiscovery_loop();
void i2cDiscovery_rescan();
void i2cDiscovery_appendJson(String& json);
bool i2cDiscovery_isPresent(const char* id);
unsigned int i2cDiscovery_getFoundCount();
