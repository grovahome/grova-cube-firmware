#include <Arduino.h>
#include "config.h"
#include "modules/i2c_discovery.h"
#include "modules/sensors.h"

#include <Wire.h>

#if GROVA_SENSOR_SHT41
  #include <Adafruit_SHT4x.h>
  Adafruit_SHT4x sht4;
#endif

#if GROVA_SENSOR_BOSCH
  #include <Adafruit_BME280.h>
  #include <Adafruit_BMP280.h>
  Adafruit_BME280 bme;
  Adafruit_BMP280 bmp(&Wire);
#endif

#if GROVA_SENSOR_VEML7700
  #include <Adafruit_VEML7700.h>
  Adafruit_VEML7700 veml;
#endif

#if GROVA_SENSOR_SCD41
  #include <SensirionI2cScd4x.h>
  SensirionI2cScd4x scd4x;
#endif

#if GROVA_SENSOR_LTR390
  #include <Adafruit_LTR390.h>
  Adafruit_LTR390 ltr390;
#endif

static const float SENSOR_TEMP_MIN = 0.0;
static const float SENSOR_TEMP_MAX = 45.0;
static const float SENSOR_HUM_MIN = 0.0;
static const float SENSOR_HUM_MAX = 100.0;
static constexpr unsigned long SENSOR_REPROBE_INTERVAL_MS = 10000UL;

static float lastT = NAN;
static float lastH = NAN;
static float lastPressure = NAN;
static float lastBoschTemp = NAN;
static float lastCo2 = NAN;
static float lastLux = NAN;
static float lastUvIndex = NAN;
static float lastScd41Temp = NAN;
static float lastScd41Hum = NAN;

static unsigned long lastRead = 0;
static unsigned long lastI2cSensorProbe = 0;
static unsigned long failCount = 0;
static unsigned int consecutiveFailCount = 0;
static bool hasPrimaryReading = false;
static SensorStatus sensorStatus = SENSOR_WAITING;
static const char* sensorSourceName = "NONE";
static const char* pressureSourceName = "NONE";
static const char* co2SourceName = "NONE";
static const char* luxSourceName = "NONE";
static const char* uvSourceName = "NONE";

#if GROVA_SENSOR_SHT41
static bool sht4Ready = false;
#endif

#if GROVA_SENSOR_BOSCH
static bool bmeReady = false;
static bool bmpReady = false;
#endif

#if GROVA_SENSOR_VEML7700
static bool vemlReady = false;
#endif

#if GROVA_SENSOR_SCD41
static bool scd41Ready = false;
static bool scd41HadReading = false;
#endif

#if GROVA_SENSOR_LTR390
static bool ltr390Ready = false;
static bool ltr390ReadUvNext = true;
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

static bool isActiveSource(const char* sourceName) {
  return strcmp(sensorSourceName, sourceName) == 0 && sensorStatus == SENSOR_OK;
}

static const char* sourceStatus(bool compiled, bool present, bool ready, bool active) {
  if (!compiled) return "disabled";
  if (!present) return "missing";
  if (active) return "active";
  if (ready) return "present";
  return "init_pending";
}

#if GROVA_SENSOR_SHT41
static void tryInitSht41(bool logResult) {
  if (sht4Ready ||
      (!i2cDiscovery_isPresent("sht4x_44") && !i2cDiscovery_isPresent("sht4x_45"))) {
    return;
  }
  sht4Ready = sht4.begin(&Wire);
  if (sht4Ready) {
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
  }
  if (logResult) Serial.println(sht4Ready ? "SHT41 ready" : "SHT41 init failed");
}
#endif

#if GROVA_SENSOR_BOSCH
static void tryInitBosch(bool logResult) {
  if (bmeReady || bmpReady ||
      (!i2cDiscovery_isPresent("bme_76") && !i2cDiscovery_isPresent("bme_77"))) {
    return;
  }

  bmeReady = bme.begin(BOSCH_PRIMARY_ADDR, &Wire);
  if (!bmeReady) bmeReady = bme.begin(BOSCH_SECONDARY_ADDR, &Wire);

  if (bmeReady) {
    pressureSourceName = "BME280";
    if (logResult) Serial.println("BME280 ready");
    return;
  }

  bmpReady = bmp.begin(BOSCH_PRIMARY_ADDR);
  if (!bmpReady) bmpReady = bmp.begin(BOSCH_SECONDARY_ADDR);

  if (bmpReady) {
    pressureSourceName = "BMP280";
    if (logResult) Serial.println("BMP280 ready");
  } else {
    pressureSourceName = "NONE";
    if (logResult) Serial.println("Bosch pressure sensor init failed");
  }
}
#endif

#if GROVA_SENSOR_VEML7700
static void tryInitVeml7700(bool logResult) {
  if (vemlReady || !i2cDiscovery_isPresent("veml7700")) return;
  vemlReady = veml.begin(&Wire);
  if (logResult) Serial.println(vemlReady ? "VEML7700 ready" : "VEML7700 init failed");
}
#endif

#if GROVA_SENSOR_SCD41
static void tryInitScd41(bool logResult) {
  if (scd41Ready || !i2cDiscovery_isPresent("scd41")) return;

  scd4x.begin(Wire, SCD41_I2C_ADDR);
  scd4x.stopPeriodicMeasurement();
  scd41Ready = scd4x.startPeriodicMeasurement() == 0;

  if (logResult) Serial.println(scd41Ready ? "SCD41 ready" : "SCD41 init failed");
}
#endif

#if GROVA_SENSOR_LTR390
static void tryInitLtr390(bool logResult) {
  if (ltr390Ready || !i2cDiscovery_isPresent("ltr390")) return;
  ltr390Ready = ltr390.begin(&Wire);
  if (ltr390Ready) {
    ltr390.enable(true);
    ltr390.setMode(LTR390_MODE_UVS);
  }
  if (logResult) Serial.println(ltr390Ready ? "LTR390 ready" : "LTR390 init failed");
}
#endif

static void tryInitI2cSensors(bool logResult) {
#if GROVA_SENSOR_SHT41
  tryInitSht41(logResult);
#endif
#if GROVA_SENSOR_BOSCH
  tryInitBosch(logResult);
#endif
#if GROVA_SENSOR_VEML7700
  tryInitVeml7700(logResult);
#endif
#if GROVA_SENSOR_SCD41
  tryInitScd41(logResult);
#endif
#if GROVA_SENSOR_LTR390
  tryInitLtr390(logResult);
#endif
}

static bool hasClimateCandidateReady() {
#if GROVA_SENSOR_SHT41
  if (sht4Ready) return true;
#endif
#if GROVA_SENSOR_SCD41
  if (scd41HadReading) return true;
#endif
#if GROVA_SENSOR_BOSCH
  if (bmeReady) return true;
#endif
  return false;
}

void sensors_begin() {
  Wire.begin(OLED_SDA, OLED_SCL);

  tryInitI2cSensors(true);
}

void sensors_loop() {
  if (millis() - lastI2cSensorProbe >= SENSOR_REPROBE_INTERVAL_MS) {
    lastI2cSensorProbe = millis();
    tryInitI2cSensors(false);
  }

  if (millis() - lastRead < 2000) return;
  lastRead = millis();

  bool primaryRead = false;
  float primaryT = NAN;
  float primaryH = NAN;
  const char* primarySource = "NONE";

#if GROVA_SENSOR_VEML7700
  if (vemlReady) {
    float lux = veml.readLux();
    if (!isnan(lux) && lux >= 0) {
      lastLux = lux;
      luxSourceName = "VEML7700";
    }
  }
#endif

#if GROVA_SENSOR_SCD41
  if (scd41Ready) {
    uint16_t co2 = 0;
    float scdTemp = NAN;
    float scdHum = NAN;
    uint16_t error = scd4x.readMeasurement(co2, scdTemp, scdHum);
    if (error == 0 && co2 > 0 && !isnan(scdTemp) && !isnan(scdHum)) {
      lastCo2 = co2;
      lastScd41Temp = scdTemp;
      lastScd41Hum = scdHum;
      co2SourceName = "SCD41";
      scd41HadReading = true;
    }
  }
#endif

#if GROVA_SENSOR_LTR390
  if (ltr390Ready) {
    if (ltr390ReadUvNext) {
      ltr390.setMode(LTR390_MODE_UVS);
      uint32_t uvRaw = ltr390.readUVS();
      if (uvRaw > 0) {
        lastUvIndex = static_cast<float>(uvRaw) / LTR390_UVI_DIVISOR;
        uvSourceName = "LTR390";
      }
    } else {
      ltr390.setMode(LTR390_MODE_ALS);
      uint32_t alsRaw = ltr390.readALS();
#if !GROVA_SENSOR_VEML7700
      if (alsRaw > 0) {
        lastLux = static_cast<float>(alsRaw);
        luxSourceName = "LTR390_ALS_RAW";
      }
#endif
    }
    ltr390ReadUvNext = !ltr390ReadUvNext;
  }
#endif

#if GROVA_SENSOR_SHT41
  if (sht4Ready) {
    sensors_event_t hum;
    sensors_event_t temp;

    if (sht4.getEvent(&hum, &temp) &&
        !isnan(temp.temperature) &&
        !isnan(hum.relative_humidity)) {
      primaryRead = true;
      primaryT = temp.temperature;
      primaryH = hum.relative_humidity;
      primarySource = "SHT41";
    }
  }
#endif

#if GROVA_SENSOR_SCD41
  if (!primaryRead && scd41HadReading) {
    primaryRead = true;
    primaryT = lastScd41Temp;
    primaryH = lastScd41Hum;
    primarySource = "SCD41";
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
    if (!hasClimateCandidateReady()) {
      setSensorStatus(SENSOR_WAITING);
      sensorSourceName = "NONE";
      return;
    }

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
float sensors_getCo2Ppm() { return lastCo2; }
float sensors_getLux() { return lastLux; }
float sensors_getUvIndex() { return lastUvIndex; }
const char* sensors_getSourceName() { return sensorSourceName; }
const char* sensors_getPressureSourceName() { return pressureSourceName; }
const char* sensors_getCo2SourceName() { return co2SourceName; }
const char* sensors_getLuxSourceName() { return luxSourceName; }
const char* sensors_getUvSourceName() { return uvSourceName; }
bool sensors_hasPressure() { return !isnan(lastPressure); }

void sensors_appendSourcesJson(String& json) {
  json += "\"sources\":[";
  bool first = true;

  auto appendSource = [&](const char* id, const char* label, const char* role, bool compiled, bool present, bool ready, bool active) {
    if (!first) json += ",";
    first = false;
    json += "{";
    json += "\"id\":\"";
    json += id;
    json += "\",\"label\":\"";
    json += label;
    json += "\",\"role\":\"";
    json += role;
    json += "\",\"compiled\":";
    json += compiled ? "true" : "false";
    json += ",\"present\":";
    json += present ? "true" : "false";
    json += ",\"active\":";
    json += active ? "true" : "false";
    json += ",\"status\":\"";
    json += sourceStatus(compiled, present, ready, active);
    json += "\"}";
  };

#if GROVA_SENSOR_SHT41
  bool shtPresent = i2cDiscovery_isPresent("sht4x_44") || i2cDiscovery_isPresent("sht4x_45");
  appendSource("sht41", "SHT41/SHT4x", "climate", true, shtPresent, sht4Ready, isActiveSource("SHT41"));
#else
  appendSource("sht41", "SHT41/SHT4x", "climate", false, false, false, false);
#endif

#if GROVA_SENSOR_SCD41
  appendSource("scd41", "SCD41", "co2_climate", true, i2cDiscovery_isPresent("scd41"), scd41Ready, isActiveSource("SCD41"));
#else
  appendSource("scd41", "SCD41", "co2_climate", false, false, false, false);
#endif

#if GROVA_SENSOR_BOSCH
  bool boschPresent = i2cDiscovery_isPresent("bme_76") || i2cDiscovery_isPresent("bme_77");
  appendSource("bme_bmp", "BME/BMP", "pressure", true, boschPresent, bmeReady || bmpReady, isActiveSource("BME280"));
#else
  appendSource("bme_bmp", "BME/BMP", "pressure", false, false, false, false);
#endif

#if GROVA_SENSOR_VEML7700
  appendSource("veml7700", "VEML7700", "light", true, i2cDiscovery_isPresent("veml7700"), vemlReady, strcmp(luxSourceName, "VEML7700") == 0);
#else
  appendSource("veml7700", "VEML7700", "light", false, false, false, false);
#endif

#if GROVA_SENSOR_LTR390
  appendSource("ltr390", "LTR390", "uv_light", true, i2cDiscovery_isPresent("ltr390"), ltr390Ready, strcmp(uvSourceName, "LTR390") == 0);
#else
  appendSource("ltr390", "LTR390", "uv_light", false, false, false, false);
#endif

  json += "]";
}

SensorStatus sensors_getStatus() { return sensorStatus; }
bool sensors_hasFault() {
  return sensorStatus == SENSOR_READ_FAIL ||
         sensorStatus == SENSOR_OUT_OF_RANGE;
}
unsigned long sensors_getFailCount() { return failCount; }
unsigned int sensors_getConsecutiveFailCount() { return consecutiveFailCount; }
