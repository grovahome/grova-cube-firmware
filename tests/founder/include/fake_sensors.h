#pragma once
#include <Arduino.h>
#include <Wire.h>
struct sensors_event_t {
  float temperature = NAN, relative_humidity = NAN;
};
struct Adafruit_AHTX0 {
  bool begin(FakeWire*, int, int) {
    return false;
  }
  bool getEvent(sensors_event_t*, sensors_event_t*) {
    return false;
  }
};
struct Adafruit_BME280 {
  bool begin(int, FakeWire*) {
    return false;
  }
  float readTemperature() {
    return NAN;
  }
  float readHumidity() {
    return NAN;
  }
  float readPressure() {
    return NAN;
  }
};
struct Adafruit_BMP280 {
  Adafruit_BMP280(FakeWire*) {}
  bool begin(int) {
    return false;
  }
  float readTemperature() {
    return NAN;
  }
  float readPressure() {
    return NAN;
  }
};
struct Adafruit_VEML7700 {
  bool begin(FakeWire*) {
    return false;
  }
  float readLux() {
    return NAN;
  }
};
struct Adafruit_LTR390 {
  bool begin(FakeWire*) {
    return false;
  }
  void enable(bool) {}
  void setMode(int) {}
  bool newDataAvailable() {
    return false;
  }
  uint32_t readUVS() {
    return 0;
  }
};
constexpr int LTR390_MODE_UVS = 1;
struct SensirionI2cScd4x {
  void begin(FakeWire&, int) {}
  int stopPeriodicMeasurement() {
    return 1;
  }
  int getSerialNumber(uint64_t&) {
    return 1;
  }
  int startPeriodicMeasurement() {
    return 1;
  }
  int getDataReadyStatus(bool&) {
    return 1;
  }
  int readMeasurement(uint16_t&, float&, float&) {
    return 1;
  }
};
