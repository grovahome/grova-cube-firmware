#pragma once

// ===== WLAN =====
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #define WIFI_SSID ""
  #define WIFI_PASS ""
#endif

// Optional local hardware profile. Copy board_config.example.h to
// board_config.h and adjust it per physical cube. The file is ignored by Git.
#if __has_include("board_config.h")
  #include "board_config.h"
#endif

// ===== OTA =====
#ifndef OTA_HOSTNAME
  #define OTA_HOSTNAME "growbox"
#endif

// ===== MQTT =====
#ifndef MQTT_HOST
  #define MQTT_HOST ""
#endif
#ifndef MQTT_PORT
  #define MQTT_PORT 1883
#endif
#ifndef MQTT_USER
  #define MQTT_USER ""
#endif
#ifndef MQTT_PASS
  #define MQTT_PASS ""
#endif
#ifndef MQTT_CUBE_ID
  #define MQTT_CUBE_ID "grova-cube-001"
#endif
#ifndef GROVA_MQTT_ENABLED
  #define GROVA_MQTT_ENABLED 1
#endif
#ifndef MQTT_DEVICE_NAME
  #define MQTT_DEVICE_NAME MQTT_CUBE_ID
#endif
#ifndef GROVA_DEVICE_MANUFACTURER
  #define GROVA_DEVICE_MANUFACTURER "GROVA"
#endif
#ifndef GROVA_DEVICE_MODEL
  #define GROVA_DEVICE_MODEL "GROVA Core Founder Edition"
#endif
#ifndef GROVA_SUPPORT_URL
  #define GROVA_SUPPORT_URL "https://github.com/grovahome/grova-cube-firmware"
#endif
#ifndef GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED
  #define GROVA_HOME_ASSISTANT_DISCOVERY_ENABLED 1
#endif
#ifndef MQTT_DISCOVERY_PREFIX
  #define MQTT_DISCOVERY_PREFIX "homeassistant"
#endif
constexpr unsigned long MQTT_TELEMETRY_INTERVAL_MS = 10000UL;
constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000UL;

// ===== LOCAL PRESET STORAGE =====
#ifndef GROVA_LOCAL_PRESET_SLOTS
  #define GROVA_LOCAL_PRESET_SLOTS 5
#endif
#ifndef GROVA_LOCAL_PRESET_MAX_PHASES
  #define GROVA_LOCAL_PRESET_MAX_PHASES 10
#endif
#ifndef GROVA_LOCAL_PRESET_MAX_PUMP_EVENTS
  #define GROVA_LOCAL_PRESET_MAX_PUMP_EVENTS 5
#endif

// ===== BOARD / SENSOR SELECTION =====
// Active firmware target: GROVA PCB v1 / Founder Edition.
// Legacy hand-wired DHT hardware is frozen and no longer updated from this
// source line; see docs/firmware-profiles.md for the last compatible commit.
#ifndef GROVA_BOARD_PCB_V1
  #define GROVA_BOARD_PCB_V1 1
#endif

#ifndef GROVA_SENSOR_SHT41
  #define GROVA_SENSOR_SHT41 1
#endif
#ifndef GROVA_SENSOR_AHT20
  #define GROVA_SENSOR_AHT20 1
#endif
#ifndef GROVA_SENSOR_BOSCH
  #define GROVA_SENSOR_BOSCH 1
#endif
#ifndef GROVA_SENSOR_VEML7700
  #define GROVA_SENSOR_VEML7700 1
#endif
#ifndef GROVA_SENSOR_SCD41
  #define GROVA_SENSOR_SCD41 1
#endif
#ifndef GROVA_SENSOR_LTR390
  #define GROVA_SENSOR_LTR390 1
#endif

#ifndef PIN_LIGHT
  #define PIN_LIGHT 26
#endif
#ifndef PIN_PUMP
  #define PIN_PUMP 13
#endif
#ifndef PIN_AUX_12V
  #define PIN_AUX_12V 27
#endif
#ifndef PIN_AUX_5V
  #define PIN_AUX_5V 14
#endif

#ifndef FAN_PWM
  #define FAN_PWM 25
#endif
#ifndef FAN_TACHO
  #define FAN_TACHO 34
#endif
#ifndef FAN2_ENABLED
  #define FAN2_ENABLED 1
#endif
#ifndef FAN2_PWM
  #define FAN2_PWM 23
#endif
#ifndef FAN2_TACHO
  #define FAN2_TACHO 35
#endif
#ifndef FAN_TACHO_ENABLED
  #define FAN_TACHO_ENABLED 1
#endif
#ifndef FAN2_TACHO_ENABLED
  #define FAN2_TACHO_ENABLED 0
#endif

#ifndef ENCODER_CLK
  #define ENCODER_CLK 33
#endif
#ifndef ENCODER_DT
  #define ENCODER_DT 32
#endif
#ifndef ENCODER_SW
  #define ENCODER_SW 2
#endif

#ifndef OLED_SDA
  #define OLED_SDA 21
#endif
#ifndef OLED_SCL
  #define OLED_SCL 22
#endif

#ifndef GROVA_HARDWARE_VERSION
  #define GROVA_HARDWARE_VERSION "GROVA PCB v1"
#endif

// ===== SENSORS =====
#ifndef BOSCH_PRIMARY_ADDR
  #define BOSCH_PRIMARY_ADDR 0x76
#endif
#ifndef BOSCH_SECONDARY_ADDR
  #define BOSCH_SECONDARY_ADDR 0x77
#endif
#ifndef AHT20_I2C_ADDR
  #define AHT20_I2C_ADDR 0x38
#endif
#ifndef VEML7700_I2C_ADDR
  #define VEML7700_I2C_ADDR 0x10
#endif
#ifndef SCD41_I2C_ADDR
  #define SCD41_I2C_ADDR 0x62
#endif
#ifndef LTR390_I2C_ADDR
  #define LTR390_I2C_ADDR 0x53
#endif
#ifndef LTR390_UVI_DIVISOR
  #define LTR390_UVI_DIVISOR 2300.0F
#endif
#ifndef OLED_ADDR
  #define OLED_ADDR 0x3C
#endif
#ifndef RTC_I2C_ADDR
  #define RTC_I2C_ADDR 0x68
#endif
#ifndef GROVA_RTC_DEFAULT_ENABLED
  #define GROVA_RTC_DEFAULT_ENABLED 0
#endif
#ifndef SENSOR_READ_FAIL_WARN_AFTER
  #define SENSOR_READ_FAIL_WARN_AFTER 3
#endif

constexpr unsigned long RTC_PROBE_INTERVAL_MS = 30000UL;
constexpr unsigned long RTC_WRITE_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

// ===== FAN =====
constexpr bool FAN_TACHO_IS_ENABLED = FAN_TACHO_ENABLED;
constexpr bool FAN2_IS_ENABLED = FAN2_ENABLED;
constexpr bool FAN2_TACHO_IS_ENABLED = FAN2_TACHO_ENABLED;
constexpr int FAN_IDLE_PERCENT = 25;
constexpr int FAN_MIN_ACTIVE_PERCENT = 35;
constexpr int FAN_MAX_PERCENT = 100;
constexpr int FAN_SENSOR_FAIL_PERCENT = 60;
constexpr int FAN_TACHO_PULSES_PER_REV = 2;
constexpr int FAN_TACHO_MIN_RPM = 100;
constexpr int FAN_TACHO_MIN_CHECK_PERCENT = 30;
constexpr unsigned long FAN_TACHO_SAMPLE_MS = 2000UL;
constexpr unsigned long FAN_TACHO_FAULT_DELAY_MS = 8000UL;
constexpr float FAN_TEMP_GAIN = 18.0;
constexpr float FAN_HUM_GAIN = 6.0;
constexpr float FAN_TEMP_DEADBAND_C = 0.4;
constexpr float FAN_HUM_DEADBAND_PERCENT = 3.0;
constexpr unsigned long FAN_RAMP_INTERVAL_MS = 250UL;
constexpr int FAN_RAMP_UP_PWM_STEP = 2;
constexpr int FAN_RAMP_DOWN_PWM_STEP = 1;
constexpr unsigned long FAN_LOG_INTERVAL_MS = 5000UL;

// ===== WARNINGS =====
constexpr float WARN_TEMP_HIGH_MARGIN_C = 3.0;
constexpr float WARN_TEMP_LOW_MARGIN_C = 4.0;
constexpr float WARN_HUM_HIGH_MARGIN_PERCENT = 12.0;
constexpr float WARN_HUM_LOW_MARGIN_PERCENT = 15.0;
constexpr unsigned long WARN_TIME_SYNC_DELAY_MS = 5UL * 60UL * 1000UL;

// ===== TIMEZONE =====
#define TIMEZONE_TZ "CET-1CEST,M3.5.0/2,M10.5.0/3"

#define LIGHT_ON_HOUR 8
#define LIGHT_OFF_HOUR 20

constexpr unsigned long DISPLAY_SLEEP_MS = 60UL * 1000UL;

// ===== PUMP =====
constexpr int PUMP_RUN_HOUR = 8;
constexpr int PUMP_RUN_MINUTE = 45;
constexpr unsigned long PUMP_RUNTIME_MS = 10UL * 1000UL;
constexpr unsigned long PUMP_TEST_RUNTIME_MS = 5UL * 1000UL;
constexpr int PUMP_RUNTIME_MIN_SECONDS = 1;
constexpr int PUMP_RUNTIME_MAX_SECONDS = 10;
constexpr unsigned long PUMP_STARTUP_LOCK_MS = 10UL * 60UL * 1000UL;
