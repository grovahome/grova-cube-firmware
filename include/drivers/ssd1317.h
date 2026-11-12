#pragma once
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Wire.h>

#include "drivers/ssd1317_protocol.h"
// Graphics and controller transport are independent of the application and mux.
// The caller owns its bus scope. Never reinitialize Wire from this driver.
class GrovaSSD1317 : public GFXcanvas1 {
 public:
  explicit GrovaSSD1317(TwoWire& bus = Wire) : GFXcanvas1(128, 64), bus_(bus) {}
  bool begin(uint8_t address, uint8_t offset = 80) {
    address_ = address;
    if (!getBuffer() || offset > 95) {
      return false;
    }
    uint8_t init[32];
    size_t count = ssd1317::initCommands(init, offset);
    if (!command(init, count)) {
      return false;
    }
    fillScreen(0);
    if (!display()) {
      return false;
    }
    delay(100);
    const uint8_t on = 0xAF;
    return command(&on, 1);
  }
  void clearDisplay() {
    fillScreen(0);
  }
  bool display() {
    if (!getBuffer()) {
      return false;
    }
    for (int page = 0; page < 8; ++page) {
      const uint8_t position[] = {uint8_t(0xB0 | page), 0x00, 0x10};
      if (!command(position, sizeof(position))) {
        return false;
      }
      // 16 data bytes per transaction also fits small host/Arduino Wire buffers.
      for (int x = 0; x < 128; x += 16) {
        bus_.beginTransmission(address_);
        bus_.write(uint8_t(0x40));
        for (int i = 0; i < 16; ++i) {
          bus_.write(ssd1317::pageByte(getBuffer(), x + i, page));
        }
        if (bus_.endTransmission() != 0) {
          return false;
        }
      }
    }
    return true;
  }

 private:
  TwoWire& bus_;
  uint8_t address_ = 0x3C;
  bool command(const uint8_t* bytes, size_t count) {
    bus_.beginTransmission(address_);
    bus_.write(uint8_t(0x00));
    for (size_t i = 0; i < count; ++i) {
      bus_.write(bytes[i]);
    }
    return bus_.endTransmission() == 0;
  }
};
