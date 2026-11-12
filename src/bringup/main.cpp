#include <Arduino.h>
#include <Wire.h>
#include "drivers/ssd1317.h"
#include "boards/grova_founder_s3.h"

// This target intentionally has no actuator commands, Wi-Fi, OTA, MQTT or runs.
static GrovaSSD1317 oled;
static bool oledReady = false;
static bool rootDevices[128] = {};
static bool muxReady = false;

static bool present(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
static bool selectMux(uint8_t mask) {
  Wire.beginTransmission(founder::MUX_ADDRESS);
  Wire.write(mask);
  return Wire.endTransmission() == 0;
}
static void parkOutputs() {
  const int pins[] = {founder::OUT12_1, founder::OUT5_1, founder::OUT12_2,
    founder::ANALOG_OUT_1, founder::ANALOG_OUT_2,
    founder::FAN1_PWM, founder::FAN2_PWM, founder::RS485_DE_RE};
  for (int pin : pins) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
}
static void scanBus() {
  muxReady = present(founder::MUX_ADDRESS);
  if (muxReady && !selectMux(0)) {
    Serial.println("ERROR: cannot isolate multiplexer channels"); return;
  }
  Serial.println("I2C root (all mux channels disabled):");
  for (int address = 1; address < 127; ++address) {
    rootDevices[address] = present(address);
    if (rootDevices[address]) Serial.printf("  root 0x%02X\n", address);
  }
  if (!muxReady) { Serial.println("TCA9548A missing"); return; }
  for (int channel = 0; channel < founder::MUX_CHANNELS; ++channel) {
    if (!selectMux(1U << channel)) {
      Serial.printf("ERROR selecting mux channel %d\n", channel); break;
    }
    for (int address = 1; address < 127; ++address) {
      if (!rootDevices[address] && present(address))
        Serial.printf("  mux[%d] 0x%02X\n", channel, address);
    }
  }
  if (!selectMux(0)) Serial.println("ERROR restoring mux isolation");
  Serial.println("Root-address collisions on downstream channels cannot be distinguished by scan.");
}
static void printStatus() {
  Serial.printf("Board: %s\nFlash: %u bytes; PSRAM: %u bytes; heap: %u bytes\n",
    GROVA_BOARD_PROFILE_NAME, ESP.getFlashChipSize(), ESP.getPsramSize(), ESP.getFreeHeap());
  Serial.printf("CONFIG=%d ENC_CLK=%d ENC_DT=%d ENC_SW=%d SAFETY=%d WATER_SAFETY=%d RTC_INT=%d\n",
    digitalRead(founder::CONFIG_BUTTON), digitalRead(founder::ENC_CLK),
    digitalRead(founder::ENC_DT), digitalRead(founder::ENC_SW),
    digitalRead(founder::SAFETY), digitalRead(founder::WATER_SAFETY), digitalRead(founder::RTC_INT));
  Serial.println("Inputs are raw electrical levels; no safety semantics assigned.");
  Serial.println("Outputs held LOW; no pump/light roles; no automation.");
}
void setup() {
  parkOutputs(); // Before USB, I2C or any display initialization.
  // Drive the addressable LED off; LOW alone does not clear a latched colour.
  neopixelWrite(founder::RGB_DATA, 0, 0, 0);
  pinMode(founder::CONFIG_BUTTON, INPUT); // External pullup on PCB.
  pinMode(founder::ENC_CLK, INPUT_PULLUP);
  pinMode(founder::ENC_DT, INPUT_PULLUP);
  pinMode(founder::ENC_SW, INPUT_PULLUP);
  pinMode(founder::SAFETY, INPUT);
  pinMode(founder::WATER_SAFETY, INPUT);
  pinMode(founder::RTC_INT, INPUT); // Do not enable pulls on the strapping pin.
  Serial.begin(115200);
  delay(1500); // Bounded startup; works without an attached USB host.
  Wire.begin(founder::SDA, founder::SCL, 100000);
  Wire.setTimeOut(50);
  printStatus();
  scanBus();
  const uint8_t oledAddress = rootDevices[0x3C] ? 0x3C : (rootDevices[0x3D] ? 0x3D : 0);
  if (oledAddress) {
    oledReady = oled.begin(oledAddress);
    if (oledReady) {
      oled.clearDisplay(); oled.setTextSize(1); oled.setTextColor(1);
      oled.setCursor(0, 0); oled.println("GROVA Founder S3");
      oled.println("BRING-UP / NO RUN"); oled.println("Outputs: LOW");
      oled.println(rootDevices[founder::RTC_ADDRESS] ? "RV3032 ACK: yes" : "RV3032 ACK: no");
      oled.display();
    }
  }
  Serial.printf("OLED: %s; RV3032 ACK: %s (clock driver not implemented)\n",
    oledReady ? "initialized, verify visually" : "unavailable",
    rootDevices[founder::RTC_ADDRESS] ? "yes" : "no");
  Serial.println("Commands: s=status, i=I2C scan. No output activation supported.");
}
void loop() {
  if (Serial.available()) {
    const char command = Serial.read();
    if (command == 's') printStatus();
    else if (command == 'i') scanBus();
  }
  delay(10);
}
