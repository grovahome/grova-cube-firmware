#include <Arduino.h>
#include <limits.h>
#include <math.h>
#include "modules/signal_registry.h"

static SignalValue signals[SIGNAL_COUNT];

static bool isValidId(SignalId id) {
  return id >= 0 && id < SIGNAL_COUNT;
}

void signalRegistry_begin() {
  for (int i = 0; i < SIGNAL_COUNT; i++) {
    signals[i] = SignalValue{};
  }
}

void signalRegistry_publish(SignalId id, float value, const char* source) {
  if (!isValidId(id)) return;
  if (isnan(value)) {
    signalRegistry_invalidate(id);
    return;
  }

  SignalValue& signal = signals[id];
  signal.value = value;
  signal.valid = true;
  signal.updatedAtMs = millis();
  signal.source = source != nullptr ? source : "UNKNOWN";
}

void signalRegistry_invalidate(SignalId id) {
  if (!isValidId(id)) return;
  signals[id].value = NAN;
  signals[id].valid = false;
  signals[id].updatedAtMs = 0;
  signals[id].source = "NONE";
}

SignalValue signalRegistry_read(SignalId id) {
  return isValidId(id) ? signals[id] : SignalValue{};
}

unsigned long signalRegistry_ageMs(SignalId id) {
  if (!isValidId(id) || !signals[id].valid) return ULONG_MAX;
  return millis() - signals[id].updatedAtMs;
}

const char* signalRegistry_name(SignalId id) {
  switch (id) {
    case SIGNAL_CLIMATE_TEMPERATURE_C: return "climate.temperature_c";
    case SIGNAL_CLIMATE_HUMIDITY_PCT: return "climate.humidity_pct";
    case SIGNAL_CLIMATE_PRESSURE_HPA: return "climate.pressure_hpa";
    case SIGNAL_CLIMATE_CO2_PPM: return "climate.co2_ppm";
    case SIGNAL_LIGHT_LUX: return "light.lux";
    case SIGNAL_LIGHT_UV_INDEX: return "light.uv_index";
    default: return "unknown";
  }
}

const char* signalRegistry_unit(SignalId id) {
  switch (id) {
    case SIGNAL_CLIMATE_TEMPERATURE_C: return "C";
    case SIGNAL_CLIMATE_HUMIDITY_PCT: return "%";
    case SIGNAL_CLIMATE_PRESSURE_HPA: return "hPa";
    case SIGNAL_CLIMATE_CO2_PPM: return "ppm";
    case SIGNAL_LIGHT_LUX: return "lx";
    case SIGNAL_LIGHT_UV_INDEX: return "index";
    default: return "";
  }
}
