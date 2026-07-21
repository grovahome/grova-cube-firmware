#include <Arduino.h>
#include "config.h"
#include "modules/sensors.h"

#if GROVA_SENSOR_DHT
  #include <DHT.h>
  DHT dht(DHTPIN, DHTTYPE);
#endif

#if GROVA_SENSOR_AHT20 || GROVA_SENSOR_BOSCH
  #include <Wire.h>
#endif

#if GROVA_SENSOR_AHT20
  #include <Adafruit_AHTX0.h>
  Adafruit_AHTX0 aht;
#endif

#if GROVA_SENSOR_BOSCH
  #include <Adafruit_BME280.h>
  #include <Adafruit_BMP280.h>
  Adafruit_BME280 bme;
  Adafruit_BMP280 bmp(&Wire);
#endif

static const float SENSOR_TEMP_MIN = 0.0;
static const float SENSOR_TEMP_MAX = 45.0;
static const float SENSOR_HUM_MIN = 0.0;
static const float SENSOR_HUM_MAX = 100.0;

static float lastT = 22.0;
static float lastH = 60.0;
static float lastPressure = NAN;
static float lastBoschTemp = NAN;

static unsigned long lastRead = 0;
static unsigned long failCount = 0;
static unsigned int consecutiveFailCount = 0;
static bool hasPrimaryReading = false;
static SensorStatus sensorStatus = SENSOR_WAITING;
static const char* sensorSourceName = "WAIT";
static const char* pressureSourceName = "NONE";

#if GROVA_SENSOR_AHT20
static bool ahtReady = false;
#endif

#if GROVA_SENSOR_BOSCH
static bool bmeReady = false;
static bool bmpReady = false;
#endif

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
#if GROVA_SENSOR_AHT20 || GROVA_SENSOR_BOSCH
  Wire.begin(OLED_SDA, OLED_SCL);
#endif

#if GROVA_SENSOR_DHT
  dht.begin();
  Serial.print("DHT enabled on pin ");
  Serial.println(DHTPIN);
#endif

#if GROVA_SENSOR_AHT20
  ahtReady = aht.begin(&Wire);
  Serial.println(ahtReady ? "AHT20 ready" : "AHT20 not found");
#endif

#if GROVA_SENSOR_BOSCH
  bmeReady = bme.begin(BOSCH_PRIMARY_ADDR, &Wire);
  if (!bmeReady) bmeReady = bme.begin(BOSCH_SECONDARY_ADDR, &Wire);

  if (bmeReady) {
    pressureSourceName = "BME280";
    Serial.println("BME280 ready");
  } else {
    bmpReady = bmp.begin(BOSCH_PRIMARY_ADDR);
    if (!bmpReady) bmpReady = bmp.begin(BOSCH_SECONDARY_ADDR);

    if (bmpReady) {
      pressureSourceName = "BMP280";
      Serial.println("BMP280 ready");
    } else {
      pressureSourceName = "NONE";
      Serial.println("Bosch pressure sensor not found");
    }
  }
#endif
}

void sensors_loop() {
  if (millis() - lastRead < 2000) return;
  lastRead = millis();

  bool primaryRead = false;
  float primaryT = NAN;
  float primaryH = NAN;
  const char* primarySource = "NONE";

#if GROVA_SENSOR_AHT20
  if (ahtReady) {
    sensors_event_t hum;
    sensors_event_t temp;
    aht.getEvent(&hum, &temp);

    if (!isnan(temp.temperature) && !isnan(hum.relative_humidity)) {
      primaryRead = true;
      primaryT = temp.temperature;
      primaryH = hum.relative_humidity;
      primarySource = "AHT20";
    }
  }
#endif

#if GROVA_SENSOR_DHT
  if (!primaryRead) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      primaryRead = true;
      primaryT = t;
      primaryH = h;
      primarySource = "DHT22";
    }
  }
#endif

#if GROVA_SENSOR_BOSCH
  if (bmeReady) {
    lastBoschTemp = bme.readTemperature();
    lastPressure = bme.readPressure() / 100.0F;

    if (!primaryRead) {
      float h = bme.readHumidity();
      if (!isnan(lastBoschTemp) && !isnan(h)) {
        primaryRead = true;
        primaryT = lastBoschTemp;
        primaryH = h;
        primarySource = "BME280";
      }
    }
  } else if (bmpReady) {
    lastBoschTemp = bmp.readTemperature();
    lastPressure = bmp.readPressure() / 100.0F;
  }
#endif

  if (!primaryRead) {
    failCount++;
    consecutiveFailCount++;
    if (!hasPrimaryReading || consecutiveFailCount >= SENSOR_READ_FAIL_WARN_AFTER) {
      setSensorStatus(SENSOR_READ_FAIL);
    }
    return;
  }

  if (!isInRange(primaryT, primaryH)) {
    failCount++;
    consecutiveFailCount++;
    if (!hasPrimaryReading || consecutiveFailCount >= SENSOR_READ_FAIL_WARN_AFTER) {
      setSensorStatus(SENSOR_OUT_OF_RANGE);
    }
    Serial.print("Sensor raw out of range | T ");
    Serial.print(primaryT, 1);
    Serial.print("C | H ");
    Serial.println(primaryH, 1);
    return;
  }

  setSensorStatus(SENSOR_OK);
  consecutiveFailCount = 0;
  hasPrimaryReading = true;
  lastT = primaryT;
  lastH = primaryH;
  sensorSourceName = primarySource;

  Serial.print("Temp: ");
  Serial.print(lastT, 1);
  Serial.print("C | Hum: ");
  Serial.print(lastH, 1);
  Serial.print("% | Source: ");
  Serial.print(sensorSourceName);
  if (!isnan(lastPressure)) {
    Serial.print(" | Pressure: ");
    Serial.print(lastPressure, 0);
    Serial.print("hPa ");
    Serial.print(pressureSourceName);
  }
  Serial.println();
}

float getTemp() { return lastT; }
float getHum() { return lastH; }

float sensors_getPressureHpa() { return lastPressure; }
float sensors_getBoschTemp() { return lastBoschTemp; }
const char* sensors_getSourceName() { return sensorSourceName; }
const char* sensors_getPressureSourceName() { return pressureSourceName; }
bool sensors_hasPressure() { return !isnan(lastPressure); }

SensorStatus sensors_getStatus() { return sensorStatus; }
bool sensors_hasFault() {
  return sensorStatus == SENSOR_READ_FAIL ||
         sensorStatus == SENSOR_OUT_OF_RANGE;
}
unsigned long sensors_getFailCount() { return failCount; }
unsigned int sensors_getConsecutiveFailCount() { return consecutiveFailCount; }
