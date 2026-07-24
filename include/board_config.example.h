#pragma once

// Copy this file to include/board_config.h and adjust it per physical cube.
// include/board_config.h is ignored by Git.
//
// Keep credentials and the unique MQTT_CUBE_ID in include/secrets.h.
// This file only describes connected hardware and pins.

// ===== Board profile =====
// 0 = legacy wiring defaults
// 1 = GROVA PCB v1 wiring defaults
#define GROVA_BOARD_PCB_V1 1
// Older local configs may still use GROVA_BOARD_PCB_V2. The firmware accepts
// that alias, but new configs should use GROVA_BOARD_PCB_V1.

// ===== Sensors =====
// Enable exactly the hardware that is fitted to this cube.
// AHT20 is the primary temperature/humidity sensor on the current PCB.
// Bosch/BME/BMP is used for pressure and diagnostic temperature.
#define GROVA_SENSOR_DHT 0
#define GROVA_SENSOR_AHT20 1
#define GROVA_SENSOR_BOSCH 1

#define DHTPIN 4
#define DHTTYPE DHT22

// Bosch sensor can be BME280 or BMP280. Firmware probes both addresses.
#define BOSCH_PRIMARY_ADDR 0x76
#define BOSCH_SECONDARY_ADDR 0x77

// ===== I2C =====
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C

// ===== MOSFET outputs =====
#define PIN_LIGHT 26
#define PIN_PUMP 13
#define PIN_AUX_12V 27
#define PIN_AUX_5V 14

// ===== Fans =====
#define FAN_PWM 25
#define FAN_TACHO 34
#define FAN_TACHO_ENABLED 1

#define FAN2_ENABLED 1
#define FAN2_PWM 23
#define FAN2_TACHO 35
#define FAN2_TACHO_ENABLED 0

// ===== Encoder =====
#define ENCODER_CLK 33
#define ENCODER_DT 32
#define ENCODER_SW 2

/*
Legacy / DHT-only example:

#define GROVA_BOARD_PCB_V1 0

#define GROVA_SENSOR_DHT 1
#define GROVA_SENSOR_AHT20 0
#define GROVA_SENSOR_BOSCH 0
#define DHTPIN 4
#define DHTTYPE DHT22

#define OLED_SDA 19
#define OLED_SCL 18
#define OLED_ADDR 0x3C

#define PIN_LIGHT 26
#define PIN_PUMP 27
#define PIN_AUX_12V -1
#define PIN_AUX_5V -1

#define FAN_PWM 25
#define FAN_TACHO 35
#define FAN_TACHO_ENABLED 0

#define FAN2_ENABLED 0
#define FAN2_PWM -1
#define FAN2_TACHO -1
#define FAN2_TACHO_ENABLED 0

#define ENCODER_CLK 32
#define ENCODER_DT 33
#define ENCODER_SW 16
*/
