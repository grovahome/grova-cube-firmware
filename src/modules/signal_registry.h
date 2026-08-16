#pragma once

#include <Arduino.h>

enum SignalId {
  SIGNAL_CLIMATE_TEMPERATURE_C = 0,
  SIGNAL_CLIMATE_HUMIDITY_PCT,
  SIGNAL_CLIMATE_PRESSURE_HPA,
  SIGNAL_CLIMATE_CO2_PPM,
  SIGNAL_LIGHT_LUX,
  SIGNAL_LIGHT_UV_INDEX,
  SIGNAL_COUNT
};

struct SignalValue {
  float value = NAN;
  bool valid = false;
  unsigned long updatedAtMs = 0;
  const char* source = "NONE";
};

void signalRegistry_begin();
void signalRegistry_publish(SignalId id, float value, const char* source);
void signalRegistry_invalidate(SignalId id);
SignalValue signalRegistry_read(SignalId id);
unsigned long signalRegistry_ageMs(SignalId id);
const char* signalRegistry_name(SignalId id);
const char* signalRegistry_unit(SignalId id);
