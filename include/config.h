#pragma once

// ===== WLAN =====
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #define WIFI_SSID ""
  #define WIFI_PASS ""
#endif

// ===== OTA =====
#define OTA_HOSTNAME "growbox"

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
constexpr unsigned long MQTT_TELEMETRY_INTERVAL_MS = 10000UL;
constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000UL;

// ===== DHT22 =====
#define DHTPIN 4

// ===== MOSFETS =====
#define PIN_LIGHT 26
#define PIN_PUMP 27

// ===== FAN =====
#define FAN_PWM 25
#define FAN_TACHO 35
constexpr bool FAN_TACHO_ENABLED = false;
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

// ===== ENCODER =====
#define ENCODER_CLK 32
#define ENCODER_DT  33
#define ENCODER_SW  16

// ===== TIMEZONE =====
#define TIMEZONE_TZ "CET-1CEST,M3.5.0/2,M10.5.0/3"

#define LIGHT_ON_HOUR 8
#define LIGHT_OFF_HOUR 20

#define OLED_SDA 19
#define OLED_SCL 18
constexpr unsigned long DISPLAY_SLEEP_MS = 60UL * 1000UL;

// ===== PUMP =====
constexpr int PUMP_RUN_HOUR = 8;
constexpr int PUMP_RUN_MINUTE = 45;
constexpr unsigned long PUMP_RUNTIME_MS = 10UL * 1000UL;
constexpr unsigned long PUMP_TEST_RUNTIME_MS = 5UL * 1000UL;
constexpr int PUMP_RUNTIME_MIN_SECONDS = 1;
constexpr int PUMP_RUNTIME_MAX_SECONDS = 10;
constexpr int PUMP_MAX_AUTO_RUNS_PER_DAY = 2;
constexpr unsigned long PUMP_MIN_AUTO_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr unsigned long PUMP_STARTUP_LOCK_MS = 10UL * 60UL * 1000UL;
