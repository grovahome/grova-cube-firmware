#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "modules/sensors.h"
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

static const float SENSOR_TEMP_MIN = 0.0;
static const float SENSOR_TEMP_MAX = 45.0;
static const float SENSOR_HUM_MIN = 0.0;
static const float SENSOR_HUM_MAX = 100.0;

static float lastT = 22.0;
static float lastH = 60.0;

static unsigned long lastRead = 0;
static unsigned long failCount = 0;
static SensorStatus sensorStatus = SENSOR_WAITING;

const char* sensors_getStatusName() {
  switch (sensorStatus) {
    case SENSOR_OK: return "OK";
    case SENSOR_READ_FAIL: return "READ_FAIL";
    case SENSOR_OUT_OF_RANGE: return "RANGE_ERR";
    case SENSOR_WAITING:
    default: return "WAIT";
  }
}

static void setSensorStatus(SensorStatus nextStatus) {
  if (sensorStatus == nextStatus) return;

  sensorStatus = nextStatus;
  Serial.print("Sensor status: ");
  Serial.println(sensors_getStatusName());
}

static bool isInRange(float t, float h) {
  return t >= SENSOR_TEMP_MIN &&
         t <= SENSOR_TEMP_MAX &&
         h >= SENSOR_HUM_MIN &&
         h <= SENSOR_HUM_MAX;
}

void sensors_begin() {
  dht.begin();
}

void sensors_loop() {
  if (millis() - lastRead < 2000) return;
  lastRead = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    failCount++;
    setSensorStatus(SENSOR_READ_FAIL);
    return;
  }

  if (!isInRange(t, h)) {
    failCount++;
    setSensorStatus(SENSOR_OUT_OF_RANGE);
    Serial.print("Sensor raw out of range | T ");
    Serial.print(t, 1);
    Serial.print("C | H ");
    Serial.println(h, 1);
    return;
  }

  setSensorStatus(SENSOR_OK);
  lastT = t;
  lastH = h;

  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print("C | Hum: ");
  Serial.println(h);
}

float getTemp() { return lastT; }
float getHum() { return lastH; }

SensorStatus sensors_getStatus() { return sensorStatus; }
bool sensors_hasFault() {
  return sensorStatus == SENSOR_READ_FAIL ||
         sensorStatus == SENSOR_OUT_OF_RANGE;
}
unsigned long sensors_getFailCount() { return failCount; }
