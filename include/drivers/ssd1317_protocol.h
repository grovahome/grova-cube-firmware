#pragma once
#include <stddef.h>
#include <stdint.h>
namespace ssd1317 {
// 128 SEG x 64 COM view, independently of how the panel is held.
constexpr int Width = 128, Height = 64, BufferSize = 1024;
// SSD1317: page addressing, 64-MUX. Offset is panel-specific (0..95).
// Reference: Wisevision X154-6428TSWXG01-H13, section 4.4.
inline size_t initCommands(uint8_t* out, uint8_t offset) {
  const uint8_t commands[] = {0xFD,   0x12, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3,
                              offset, 0x40, 0x20, 0x02, 0xA1, 0xC0, 0xDA, 0x12,
                              0x81,   0x35, 0xD9, 0x22, 0xDB, 0x30, 0xA4, 0xA6};
  for (size_t i = 0; i < sizeof(commands); ++i) {
    out[i] = commands[i];
  }
  return sizeof(commands);
}
inline uint8_t pageByte(const uint8_t* canvas, int x, int page) {
  uint8_t value = 0;
  for (int bit = 0; bit < 8; ++bit) {
    int y = page * 8 + bit;
    if (canvas[y * (Width / 8) + x / 8] & (0x80 >> (x % 8))) {
      value |= 1U << bit;
    }
  }
  return value;
}
// Turn the landscape canvas into 16 native pages of 64 columns for SH1107.
inline uint8_t portraitPageByte(const uint8_t* canvas, int column, int page) {
  const int y = Height - 1 - column;
  const uint8_t pixels = canvas[y * (Width / 8) + page];
  uint8_t value = 0;
  for (int bit = 0; bit < 8; ++bit) {
    if (pixels & (0x80 >> bit)) {
      value |= 1U << bit;
    }
  }
  return value;
}
}  // namespace ssd1317
