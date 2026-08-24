#pragma once

// Compile-time identity for the existing GROVA PCB v1 hardware line.
#define GROVA_BOARD_PROFILE_ID "grova_core_v1"
#define GROVA_BOARD_PROFILE_NAME "GROVA PCB v1"
#define GROVA_BOARD_MCU_NAME "ESP32"

// Compatibility alias used by existing local board configurations.
#ifndef GROVA_BOARD_PCB_V1
  #define GROVA_BOARD_PCB_V1 1
#endif

// Board capabilities. These describe physical hardware, not enabled runtime
// policies. They can later be consumed by APIs and shared modules without
// inferring hardware from a cube ID.
#define GROVA_BOARD_FAN_COUNT 2
#define GROVA_BOARD_I2C_BUS_COUNT 1
#define GROVA_BOARD_HAS_AUX_12V 1
#define GROVA_BOARD_HAS_AUX_5V 1
#define GROVA_BOARD_HAS_ANALOG_0_10V 0
#define GROVA_BOARD_HAS_ANALOG_INPUTS 0
#define GROVA_BOARD_HAS_FLOW_INPUT 0
#define GROVA_BOARD_HAS_SAFETY_INPUT 0
#define GROVA_BOARD_HAS_ONEWIRE 0
#define GROVA_BOARD_HAS_RS485 0

// MOSFET outputs.
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

// Fans.
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

// Rotary encoder.
#ifndef ENCODER_CLK
  #define ENCODER_CLK 33
#endif
#ifndef ENCODER_DT
  #define ENCODER_DT 32
#endif
#ifndef ENCODER_SW
  #define ENCODER_SW 2
#endif

// System I2C bus used by the display, sensors, and optional RTC.
#ifndef OLED_SDA
  #define OLED_SDA 21
#endif
#ifndef OLED_SCL
  #define OLED_SCL 22
#endif

#ifndef GROVA_HARDWARE_VERSION
  #define GROVA_HARDWARE_VERSION GROVA_BOARD_PROFILE_NAME
#endif
