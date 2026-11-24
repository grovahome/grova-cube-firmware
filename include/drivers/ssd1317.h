#pragma once
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Wire.h>

#include "drivers/ssd1317_protocol.h"

// Values are stored in the two high bits of the existing OLED setting.
enum class GrovaOledProfile : uint16_t {
  Ssd1317 = 0x0000,
  Ssd1312 = 0x4000,
  Ssd1306 = 0x8000,
  Sh1107 = 0xC000,
};

inline unsigned oledMaxOffset(GrovaOledProfile profile) {
  switch (profile) {
    case GrovaOledProfile::Ssd1317: return 95;
    case GrovaOledProfile::Sh1107: return 127;
    default: return 63;
  }
}
// Graphics and controller transport are independent of the application and mux.
// The caller owns its bus scope. Never reinitialize Wire from this driver.
class GrovaSSD1317 : public GFXcanvas1 {
 public:
  explicit GrovaSSD1317(TwoWire& bus = Wire) : GFXcanvas1(128, 64), bus_(bus) {}
  bool begin(uint8_t address, uint8_t offset = 80, GrovaOledProfile profile = GrovaOledProfile::Ssd1317) {
    address_ = address;
    if (!getBuffer() || offset > oledMaxOffset(profile)) {
      return false;
    }
    profile_ = profile;
    if (!initialize(offset)) {
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
    const bool portrait = profile_ == GrovaOledProfile::Sh1107;
    const int pages = portrait ? 16 : 8;
    const int columns = portrait ? 64 : 128;
    const auto pageByte = portrait ? ssd1317::portraitPageByte : ssd1317::pageByte;
    for (int page = 0; page < pages; ++page) {
      const uint8_t position[] = {uint8_t(0xB0 | page), 0x00, 0x10};
      if (!command(position, sizeof(position))) {
        return false;
      }
      // 16 data bytes per transaction also fits small host/Arduino Wire buffers.
      for (int x = 0; x < columns; x += 16) {
        bus_.beginTransmission(address_);
        bus_.write(uint8_t(0x40));
        for (int i = 0; i < 16; ++i) {
          bus_.write(pageByte(getBuffer(), x + i, page));
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
  GrovaOledProfile profile_ = GrovaOledProfile::Ssd1317;

  bool initialize(uint8_t offset) {
    if (profile_ == GrovaOledProfile::Sh1107) {
      // SH1107 64x128 panel wiring, as used by the Adafruit OLED FeatherWing.
      const uint8_t commands[] = {0xAE, 0xD5, 0x51, 0x20, 0x81, 0x4F, 0xAD, 0x8A,
        0xA0, 0xC0, 0xDC, 0x00, 0xD3, offset, 0xD9, 0x22, 0xDB, 0x35, 0xA8, 0x3F,
        0xA4, 0xA6};
      return command(commands, sizeof(commands));
    }
    if (profile_ == GrovaOledProfile::Ssd1312) {
      // SSD1312 command table: COM page addressing, alternate SEG pin wiring
      // and internal charge pump at its lowest output (7.5 V).
      const uint8_t commands[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, offset,
        0x40, 0x8D, 0x16, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x10,
        0x81, 0x7F, 0xD9, 0x22, 0xDB, 0x30, 0xA4, 0xA6};
      return command(commands, sizeof(commands));
    }
    if (profile_ == GrovaOledProfile::Ssd1306) {
      // SSD1306 128x64, internal charge pump, page addressing for this transport.
      // Register settings follow Adafruit_SSD1306; SSD1317 keeps its own profile.
      const uint8_t commands[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, offset,
        0x40, 0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x2E};
      return command(commands, sizeof(commands));
    }
    uint8_t commands[32];
    const size_t count = ssd1317::initCommands(commands, offset);
    return command(commands, count);
  }

  bool command(const uint8_t* bytes, size_t count) {
    bus_.beginTransmission(address_);
    bus_.write(uint8_t(0x00));
    for (size_t i = 0; i < count; ++i) {
      bus_.write(bytes[i]);
    }
    return bus_.endTransmission() == 0;
  }
};
