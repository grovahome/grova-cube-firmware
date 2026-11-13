#include "founder/devices.h"

#include <Adafruit_AHTX0.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_LTR390.h>
#include <Adafruit_VEML7700.h>
#include <SensirionI2cScd4x.h>
#include <Wire.h>
#include <math.h>

#include <new>

#include "founder/i2c_bus.h"
#include "modules/sensors.h"
#include "modules/signal_registry.h"
namespace founderDevices {
enum Type { SHT, AHT, BOSCH, VEML, SCD, LTR };
struct Candidate {
  Type type;
  uint8_t address;
};
static const Candidate candidates[] = {{SHT, 0x44},   {SHT, 0x45},  {AHT, 0x38}, {BOSCH, 0x76},
                                       {BOSCH, 0x77}, {VEML, 0x10}, {SCD, 0x62}, {LTR, 0x53}};
struct Device {
  int lane;
  Type type;
  uint8_t address;
  bool present = false, ready = false, valid = false;
  uint32_t sampled = 0, probeAt = 0;
  bool probed = false;
  float temperature = NAN, humidity = NAN, extra = NAN;
  char source[40] = {};
  Adafruit_AHTX0* aht = nullptr;
  Adafruit_BME280* bme = nullptr;
  Adafruit_BMP280* bmp = nullptr;
  Adafruit_VEML7700* veml = nullptr;
  SensirionI2cScd4x* scd = nullptr;
  Adafruit_LTR390* ltr = nullptr;
};
static Device devices[72];
static bool root[128] = {};
static uint32_t refreshed = 0;
static unsigned cursor = 0;
static const char* name(const Device& d) {
  switch (d.type) {
    case SHT:
      return "SHT4x";
    case AHT:
      return "AHT20";
    case BOSCH:
      return d.bmp ? "BMP280" : "BME280";
    case VEML:
      return "VEML7700";
    case SCD:
      return "SCD41";
    case LTR:
      return "LTR390";
  }
  return "unknown";
}
static uint8_t crc(const uint8_t* b) {
  uint8_t c = 0xff;
  for (int i = 0; i < 2; ++i) {
    c ^= b[i];
    for (int j = 0; j < 8; ++j) {
      c = (c & 0x80) ? (c << 1) ^ 0x31 : c << 1;
    }
  }
  return c;
}
static bool shtCommand(Device& d, uint8_t cmd, uint8_t* data) {
  Wire.beginTransmission(d.address);
  Wire.write(cmd);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(10);
  if (Wire.requestFrom(d.address, uint8_t(6)) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    data[i] = Wire.read();
  }
  return crc(data) == data[2] && crc(data + 3) == data[5];
}
static void release(Device& d) {
  delete d.aht;
  delete d.bme;
  delete d.bmp;
  delete d.veml;
  delete d.scd;
  delete d.ltr;
  d.aht = nullptr;
  d.bme = nullptr;
  d.bmp = nullptr;
  d.veml = nullptr;
  d.scd = nullptr;
  d.ltr = nullptr;
  d.ready = d.valid = false;
  d.temperature = d.humidity = d.extra = NAN;
}
static bool init(Device& d) {
  release(d);
  uint8_t bytes[6];
  bool ok = false;
  switch (d.type) {
    case SHT:
      ok = shtCommand(d, 0x89, bytes);
      break;  // Serial number + CRC identifies supported protocol.
    case AHT:
      d.aht = new (std::nothrow) Adafruit_AHTX0();
      ok = d.aht && d.aht->begin(&Wire, 0, d.address);
      break;
    case BOSCH:
      d.bme = new (std::nothrow) Adafruit_BME280();
      ok = d.bme && d.bme->begin(d.address, &Wire);
      if (!ok) {
        delete d.bme;
        d.bme = nullptr;
        d.bmp = new (std::nothrow) Adafruit_BMP280(&Wire);
        ok = d.bmp && d.bmp->begin(d.address);
      }
      break;
    case VEML:
      d.veml = new (std::nothrow) Adafruit_VEML7700();
      ok = d.veml && d.veml->begin(&Wire);
      break;
    case SCD:
      d.scd = new (std::nothrow) SensirionI2cScd4x();
      if (d.scd) {
        d.scd->begin(Wire, d.address);
        d.scd->stopPeriodicMeasurement();
        delay(500);
        uint64_t serial = 0;
        ok = d.scd->getSerialNumber(serial) == 0;
        if (ok) {
          ok = d.scd->startPeriodicMeasurement() == 0;
        }
      }
      break;
    case LTR:
      d.ltr = new (std::nothrow) Adafruit_LTR390();
      ok = d.ltr && d.ltr->begin(&Wire);
      if (ok) {
        d.ltr->enable(true);
        d.ltr->setMode(LTR390_MODE_UVS);
      }
      break;
  }
  if (!ok) {
    release(d);
  }
  d.ready = ok;
  snprintf(d.source, sizeof(d.source), "%s@%s%d/0x%02X", name(d), d.lane < 0 ? "root" : "ch",
           d.lane < 0 ? 0 : d.lane, d.address);
  return ok;
}
static bool sample(Device& d) {
  float t = NAN, h = NAN, x = NAN;
  bool ok = false;
  switch (d.type) {
    case SHT: {
      uint8_t b[6];
      ok = shtCommand(d, 0xFD, b);
      if (ok) {
        t = -45.0f + 175.0f * uint16_t((b[0] << 8) | b[1]) / 65535.0f;
        h = constrain(-6.0f + 125.0f * uint16_t((b[3] << 8) | b[4]) / 65535.0f, 0.0f, 100.0f);
      }
    } break;
    case AHT: {
      sensors_event_t he, te;
      ok = d.aht->getEvent(&he, &te);
      if (ok) {
        t = te.temperature;
        h = he.relative_humidity;
      }
    } break;
    case BOSCH:
      if (d.bme) {
        t = d.bme->readTemperature();
        h = d.bme->readHumidity();
        x = d.bme->readPressure() / 100.0f;
      } else {
        t = d.bmp->readTemperature();
        x = d.bmp->readPressure() / 100.0f;
      }
      ok = isfinite(t) && isfinite(x) && x > 0;
      break;
    case VEML:
      x = d.veml->readLux();
      ok = isfinite(x) && x >= 0;
      break;
    case SCD: {
      uint16_t co2 = 0;
      bool ready = false;
      if (d.scd->getDataReadyStatus(ready) != 0) {
        return false;
      }
      if (!ready) {
        return d.valid;  // Preserve real sample timestamp until new data arrives.
      }
      ok = d.scd->readMeasurement(co2, t, h) == 0 && co2 > 0;
      x = co2;
    } break;
    case LTR:
      if (!d.ltr->newDataAvailable()) {
        return d.valid;
      }
      x = d.ltr->readUVS() / 2300.0f;
      ok = isfinite(x) && x >= 0;
      break;
  }
  if ((d.type == SHT || d.type == AHT || d.type == SCD) && (!isfinite(t) || !isfinite(h))) {
    ok = false;
  }
  if ((!isnan(t) && (!isfinite(t) || t < -40 || t > 85)) ||
      (!isnan(h) && (!isfinite(h) || h < 0 || h > 100))) {
    ok = false;
  }
  if (ok) {
    d.temperature = t;
    d.humidity = h;
    d.extra = x;
    d.sampled = millis();
    d.valid = true;
  } else {
    d.valid = false;
    d.ready = false;
  }
  return ok;
}
static bool usable(const Device& d) {
  return d.valid && millis() - d.sampled < 12000;
}
static void publish() {
  const Device* bestT = nullptr;
  const Device* bestH = nullptr;
  const Device* pressure = nullptr;
  const Device* co2 = nullptr;
  const Device* lux = nullptr;
  const Device* uv = nullptr;
  auto rank = [](const Device& d) {
    return d.type == SHT ? 0 : d.type == AHT ? 1 : d.type == SCD ? 2 : 3;
  };
  for (const Device& d : devices) {
    if (usable(d)) {
      if (isfinite(d.temperature) && (!bestT || rank(d) < rank(*bestT))) {
        bestT = &d;
      }
      if (isfinite(d.humidity) && (!bestH || rank(d) < rank(*bestH))) {
        bestH = &d;
      }
      if (d.type == BOSCH && !pressure) {
        pressure = &d;
      }
      if (d.type == SCD && !co2) {
        co2 = &d;
      }
      if (d.type == VEML && !lux) {
        lux = &d;
      }
      if (d.type == LTR && !uv) {
        uv = &d;
      }
    }
  }
  auto signal = [](SignalId id, const Device* d, float value) {
    if (!d) {
      signalRegistry_invalidate(id);
      return;
    }
    // Do not make an old sample look fresh by publishing it on every loop.
    SignalValue current = signalRegistry_read(id);
    if (current.source != d->source || current.updatedAtMs != d->sampled) {
      signalRegistry_publishAt(id, value, d->source, d->sampled);
    }
  };
  signal(SIGNAL_CLIMATE_TEMPERATURE_C, bestT, bestT ? bestT->temperature : NAN);
  signal(SIGNAL_CLIMATE_HUMIDITY_PCT, bestH, bestH ? bestH->humidity : NAN);
  signal(SIGNAL_CLIMATE_PRESSURE_HPA, pressure, pressure ? pressure->extra : NAN);
  signal(SIGNAL_CLIMATE_CO2_PPM, co2, co2 ? co2->extra : NAN);
  signal(SIGNAL_LIGHT_LUX, lux, lux ? lux->extra : NAN);
  signal(SIGNAL_LIGHT_UV_INDEX, uv, uv ? uv->extra : NAN);
}
void begin() {
  int i = 0;
  for (int lane = -1; lane < 8; ++lane) {
    for (const Candidate& c : candidates) {
      Device& d = devices[i++];
      d.lane = lane;
      d.type = c.type;
      d.address = c.address;
    }
  }
  refreshed = millis() - 10000;
}
void loop() {
  if (millis() - refreshed >= 10000) {
    refreshed = millis();
    founderBus::refresh();
    founderBus::Scope bus;
    for (const Candidate& c : candidates) {
      root[c.address] = bool(bus) && founderBus::probe(c.address);
    }
  }
  Device& d = devices[cursor++ % 72];
  if (d.lane >= 0 && (!founderBus::hasMux() || root[d.address])) {
    if (d.ready || d.valid) {
      release(d);
    }
    d.present = false;
    publish();
    return;
  }
  bool probeDue = !d.probed || millis() - d.probeAt >= 10000;
  if (probeDue || (d.ready && millis() - d.sampled >= 2000)) {
    founderBus::Scope bus(d.lane);
    if (probeDue) {
      d.probed = true;
      d.probeAt = millis();
      d.present = bool(bus) && founderBus::probe(d.address);
    }
    if (!bus || !d.present) {
      if (d.ready || d.valid) {
        release(d);
      }
      d.present = false;
    } else {
      if (!d.ready && probeDue) {
        init(d);
      }
      if (d.ready && millis() - d.sampled >= 2000) {
        sample(d);
      }
    }
  }
  publish();
}
static float measurement(const Device& d, SignalId id) {
  if (id == SIGNAL_CLIMATE_TEMPERATURE_C) {
    return d.temperature;
  }
  if (id == SIGNAL_CLIMATE_HUMIDITY_PCT) {
    return d.humidity;
  }
  if ((id == SIGNAL_CLIMATE_PRESSURE_HPA && d.type == BOSCH) ||
      (id == SIGNAL_CLIMATE_CO2_PPM && d.type == SCD) ||
      (id == SIGNAL_LIGHT_LUX && d.type == VEML) ||
      (id == SIGNAL_LIGHT_UV_INDEX && d.type == LTR)) {
    return d.extra;
  }
  return NAN;
}
static bool supported(const Device& d, SignalId id) {
  return (id == SIGNAL_CLIMATE_TEMPERATURE_C &&
          (d.type == SHT || d.type == AHT || d.type == BOSCH || d.type == SCD)) ||
         (id == SIGNAL_CLIMATE_HUMIDITY_PCT &&
          (d.type == SHT || d.type == AHT || (d.type == BOSCH && !d.bmp) || d.type == SCD)) ||
         (id == SIGNAL_CLIMATE_PRESSURE_HPA && d.type == BOSCH) ||
         (id == SIGNAL_CLIMATE_CO2_PPM && d.type == SCD) ||
         (id == SIGNAL_LIGHT_LUX && d.type == VEML) ||
         (id == SIGNAL_LIGHT_UV_INDEX && d.type == LTR);
}
void forEachSignal(MeasurementVisitor visitor) {
  for (const auto& d : devices) {
    if (!d.present || !*d.source) {
      continue;
    }
    for (int n = 0; n < SIGNAL_COUNT; ++n) {
      auto id = static_cast<SignalId>(n);
      if (!supported(d, id)) {
        continue;
      }
      const float value = measurement(d, id);
      visitor(d.source, id, value, usable(d) && isfinite(value));
    }
  }
}
founderAutomation::Reading readSignal(SignalId id, const char* source) {
  for (const auto& d : devices) {
    if (d.present && !strcmp(d.source, source)) {
      const float v = measurement(d, id);
      return {v, usable(d) && isfinite(v), uint32_t(millis() - d.sampled)};
    }
  }
  return {};
}
void appendJson(String& j) {
  j += "\"devices\":[";
  bool comma = false;
  for (const Device& d : devices) {
    if (d.present) {
      if (comma) {
        j += ',';
      }
      comma = true;
      j += "{\"type\":\"";
      j += name(d);
      j += "\",\"channel\":";
      j += d.lane;
      j += " ,\"address\":";
      j += d.address;
      j += " ,\"ready\":";
      j += d.ready ? "true" : "false";
      j += " ,\"valid\":";
      j += usable(d) ? "true" : "false";
      j += " ,\"age_ms\":";
      if (d.valid) {
        j += millis() - d.sampled;
      } else {
        j += "null";
      }
      j += " ,\"temperature_c\":";
      j += usable(d) && isfinite(d.temperature) ? String(d.temperature, 2) : String("null");
      j += " ,\"humidity_pct\":";
      j += usable(d) && isfinite(d.humidity) ? String(d.humidity, 2) : String("null");
      j += ",\"source\":\"";
      j += d.source;
      j += "\",\"signals\":[";
      bool signalComma = false;
      for (int id = 0; id < SIGNAL_COUNT; ++id) {
        const auto signal = static_cast<SignalId>(id);
        const float value = measurement(d, signal);
        if (!supported(d, signal)) {
          continue;
        }
        if (signalComma) {
          j += ',';
        }
        signalComma = true;
        j += "{\"id\":\"";
        j += signalRegistry_name(signal);
        j += "\",\"unit\":\"";
        j += signalRegistry_unit(signal);
        j += "\",\"valid\":";
        j += usable(d) && isfinite(value) ? "true" : "false";
        j += ",\"value\":";
        j += usable(d) && isfinite(value) ? String(value, 3) : String("null");
        j += '}';
      }
      j += "]}";
    }
  }
  j += ']';
}
}  // namespace founderDevices
// Shared fan layer depends on this abstract health contract, never a sensor driver.
bool sensors_hasFault() {
  return !signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C).valid &&
         !signalRegistry_read(SIGNAL_CLIMATE_HUMIDITY_PCT).valid;
}
