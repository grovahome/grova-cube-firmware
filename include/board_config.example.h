#pragma once

// Copy this file to include/board_config.h and adjust it per physical cube.
// include/board_config.h is ignored by Git.
//
// Keep credentials and the unique MQTT_CUBE_ID in include/secrets.h.
// This file only describes connected hardware and pins.

// The active board profile and its default pins live in:
//   include/boards/grova_core_v1.h
//
// This optional local file is only for deviations on an individual cube.
// Leave a setting undefined to use the versioned V1 board-profile default.

// ===== Sensors =====
// Enable the driver families that this firmware may auto-detect on the cube.
// Missing I2C sensors are allowed; they are reported as missing/unavailable.
// Preferred temperature/humidity order:
// SHT41/SHT4x -> AHT20/AHTx0 -> SCD41 -> BME280.
#define GROVA_SENSOR_SHT41 1
#define GROVA_SENSOR_AHT20 1
#define GROVA_SENSOR_BOSCH 1
#define GROVA_SENSOR_VEML7700 1
#define GROVA_SENSOR_SCD41 1
#define GROVA_SENSOR_LTR390 1

// Bosch sensor can be BME280 or BMP280. Firmware probes both addresses.
#define BOSCH_PRIMARY_ADDR 0x76
#define BOSCH_SECONDARY_ADDR 0x77
#define AHT20_I2C_ADDR 0x38
#define VEML7700_I2C_ADDR 0x10
#define SCD41_I2C_ADDR 0x62
#define LTR390_I2C_ADDR 0x53
#define LTR390_UVI_DIVISOR 2300.0F

// ===== Optional I2C overrides =====
// #define OLED_SDA 21
// #define OLED_SCL 22
#define OLED_ADDR 0x3C

// Optional DS3231/DS1307-compatible RTC on the same I2C bus.
// Runtime default is off; enable it from the ESP web status page or API when fitted.
#define RTC_I2C_ADDR 0x68
#define GROVA_RTC_DEFAULT_ENABLED 0

// ===== Optional MOSFET output overrides =====
// #define PIN_LIGHT 26
// #define PIN_PUMP 13
// #define PIN_AUX_12V 27
// #define PIN_AUX_5V 14

// ===== Optional fan overrides =====
// #define FAN_PWM 25
// #define FAN_TACHO 34
// #define FAN_TACHO_ENABLED 1

// #define FAN2_ENABLED 1
// #define FAN2_PWM 23
// #define FAN2_TACHO 35
// #define FAN2_TACHO_ENABLED 0

// ===== Optional encoder overrides =====
// #define ENCODER_CLK 33
// #define ENCODER_DT 32
// #define ENCODER_SW 2
