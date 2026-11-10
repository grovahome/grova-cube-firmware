#pragma once

// SCH_Schematic1_2026-11-01.pdf, U2 net labels (GPIO, not module pad numbers).
#define GROVA_BOARD_PROFILE_ID "grova_founder_s3"
#define GROVA_BOARD_PROFILE_NAME "GROVA Founder ESP32-S3"
#define GROVA_BOARD_MCU_NAME "ESP32-S3-WROOM-1-N16R8"
namespace founder {
constexpr int OUT12_1 = 4, OUT5_1 = 5, OUT12_2 = 6;
constexpr int ANALOG_OUT_1 = 7, ANALOG_OUT_2 = 40;
constexpr int SDA = 8, SCL = 9;
constexpr int FAN1_PWM = 10, FAN2_PWM = 11;
constexpr int FAN1_TACHO = 12, FAN2_TACHO = 13;
constexpr int ENC_CLK = 14, ENC_DT = 15, ENC_SW = 16;
constexpr int ONEWIRE = 17;
constexpr int ANALOG_IN_1 = 1, ANALOG_IN_2 = 2;
constexpr int SAFETY = 38, WATER_SAFETY = 39;
constexpr int RS485_TX = 41, RS485_RX = 42, RS485_DE_RE = 47;
constexpr int UART_TX = 43, UART_RX = 44;
constexpr int RGB_DATA = 21, CONFIG_BUTTON = 48;
constexpr int USB_DM = 19, USB_DP = 20;
constexpr int RTC_INT = 46; // Strapping pin: verify external level at reset.
constexpr unsigned char MUX_ADDRESS = 0x70;
constexpr unsigned char RTC_ADDRESS = 0x51; // RV-3032; detection only for now.
constexpr int MUX_CHANNELS = 8;
// No pump/light role mapping. GPIO18 is unconnected in this schematic.
}

#if defined(GROVA_FOUNDER_APP)
#define FAN_PWM 10
#define FAN2_PWM 11
#define FAN_TACHO 12
#define FAN2_TACHO 13
#define FAN2_ENABLED 1
#define FAN_TACHO_ENABLED 1
#define FAN2_TACHO_ENABLED 1
#define OLED_SDA 8
#define OLED_SCL 9
#define GROVA_HARDWARE_VERSION GROVA_BOARD_PROFILE_NAME
#define GROVA_MQTT_ENABLED 0
#define OTA_HOSTNAME "grova-founder"
#endif
