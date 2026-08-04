#include <Arduino.h>
#include <Wire.h>
#include <string.h>

#include "config.h"
#include "modules/i2c_discovery.h"

static constexpr unsigned long I2C_DISCOVERY_RESCAN_MS = 60000UL;

static I2cKnownDevice knownDevices[] = {
  {"sht4x_44", "SHT41/SHT4x", "GROVA Climate", 0x44, false},
  {"sht4x_45", "SHT41/SHT4x", "GROVA Climate", 0x45, false},
  {"aht20", "AHT20/AHTx0", "GROVA PCB v1 Climate", 0x38, false},
  {"veml7700", "VEML7700", "GROVA Light", 0x10, false},
  {"scd41", "SCD41", "GROVA CO2", 0x62, false},
  {"bme_76", "BME/BMP", "GROVA Pressure", 0x76, false},
  {"bme_77", "BME/BMP", "GROVA Pressure", 0x77, false},
  {"ltr390", "LTR390", "GROVA UV", 0x53, false},
  {"oled_3c", "SSD1306 OLED", "System Display", 0x3C, false},
  {"oled_3d", "SSD1306 OLED", "System Display", 0x3D, false},
  {"rtc_68", "DS3231/DS1307 RTC", "System RTC", 0x68, false},
};

static unsigned long lastScan = 0;
static unsigned int foundCount = 0;

static bool probeAddress(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void i2cDiscovery_rescan() {
  foundCount = 0;
  for (I2cKnownDevice& device : knownDevices) {
    device.present = probeAddress(device.address);
    if (device.present) foundCount++;
  }
  lastScan = millis();

  Serial.print("I2C discovery: ");
  Serial.print(foundCount);
  Serial.println(" known device address(es) found");
}

void i2cDiscovery_begin() {
  Wire.begin(OLED_SDA, OLED_SCL);
  i2cDiscovery_rescan();
}

void i2cDiscovery_loop() {
  if (millis() - lastScan >= I2C_DISCOVERY_RESCAN_MS) {
    i2cDiscovery_rescan();
  }
}

bool i2cDiscovery_isPresent(const char* id) {
  for (const I2cKnownDevice& device : knownDevices) {
    if (strcmp(device.id, id) == 0) return device.present;
  }
  return false;
}

unsigned int i2cDiscovery_getFoundCount() {
  return foundCount;
}

void i2cDiscovery_appendJson(String& json) {
  json += "\"i2c\":{";
  json += "\"sda\":";
  json += OLED_SDA;
  json += ",\"scl\":";
  json += OLED_SCL;
  json += ",\"found_count\":";
  json += foundCount;
  json += ",\"devices\":[";
  for (size_t i = 0; i < sizeof(knownDevices) / sizeof(knownDevices[0]); i++) {
    const I2cKnownDevice& device = knownDevices[i];
    if (i > 0) json += ",";
    json += "{";
    json += "\"id\":\"";
    json += device.id;
    json += "\",\"name\":\"";
    json += device.name;
    json += "\",\"module\":\"";
    json += device.module;
    json += "\",\"addr\":\"0x";
    if (device.address < 16) json += "0";
    json += String(device.address, HEX);
    json += "\",\"present\":";
    json += device.present ? "true" : "false";
    json += "}";
  }
  json += "]}";
}
