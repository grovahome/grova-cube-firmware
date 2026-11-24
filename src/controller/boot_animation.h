#pragma once
#include <stdint.h>
// Time-driven welcome graphics only: the bar is not a hardware-readiness measurement.
namespace founderBoot {
constexpr uint32_t DurationMs = 2200;
constexpr uint32_t FrameMs = 100;
constexpr uint32_t StartWindowMs = 8000;
// Compact monochrome G/leaf mark. Drawn in code; no external assets or allocation.
constexpr uint32_t Mark[24] = {0x000078, 0x0001F0, 0x0007E0, 0x000FC0, 0x001F80, 0x003F00,
                               0x007E00, 0x00FC00, 0x03F800, 0x0FF000, 0x1F8000, 0x3C0000,
                               0x780000, 0x700000, 0xE00000, 0xE03FF0, 0xE03FF0, 0xE00070,
                               0x7000E0, 0x7801E0, 0x3C07C0, 0x1FFF80, 0x0FFF00, 0x03FC00};
constexpr uint8_t Letters[8][5] = {{0x3E, 0x41, 0x49, 0x49, 0x3A},  // G
                                   {0x7F, 0x09, 0x19, 0x29, 0x46},  // R
                                   {0x3E, 0x41, 0x41, 0x41, 0x3E},  // O
                                   {0x1F, 0x20, 0x40, 0x20, 0x1F},  // V
                                   {0x7C, 0x02, 0x01, 0x02, 0x7C},  // A as inverted V (GROVA wordmark)
                                   {0x3E, 0x41, 0x41, 0x41, 0x22},  // C
                                   {0x7F, 0x49, 0x49, 0x49, 0x41},  // E
                                   {0, 0, 0, 0, 0}};
template <class Canvas>
void rect(Canvas& c, int x, int y, int w, int h, bool fill) {
  for (int yy = 0; yy < h; ++yy) {
    for (int xx = 0; xx < w; ++xx) {
      if (fill || xx == 0 || yy == 0 || xx == w - 1 || yy == h - 1) {
        c.drawPixel(x + xx, y + yy, 1);
      }
    }
  }
}
template <class Canvas>
void letter(Canvas& c, unsigned index, int x, int y, int scale) {
  for (int xx = 0; xx < 5; ++xx) {
    for (int yy = 0; yy < 7; ++yy) {
      if (Letters[index][xx] & (1U << yy)) {
        rect(c, x + xx * scale, y + yy * scale, scale, scale, true);
      }
    }
  }
}
template <class Canvas>
void draw(Canvas& c, uint32_t elapsed, int xOffset = 0) {
  const bool portrait = c.width() < c.height();
  const int markX = (portrait ? (c.width() - 24) / 2 : 14) + xOffset, markY = portrait ? 18 : 12;
  const unsigned rows = elapsed >= 480 ? 24 : elapsed * 24 / 480;
  for (unsigned y = 24 - rows; y < 24; ++y) {
    for (int x = 0; x < 24; ++x) {
      if (Mark[y] & (1UL << (23 - x))) {
        c.drawPixel(markX + x, markY + y, 1);
      }
    }
  }
  const int titleX = (portrait ? (c.width() - 58) / 2 : 49) + xOffset, titleY = portrait ? 55 : 17;
  for (unsigned i = 0; i < 5; ++i) {
    if (elapsed >= 220 + i * 120) {
      letter(c, i, titleX + i * 12, titleY, 2);
    }
  }
  if (elapsed >= 850) {
    const unsigned core[] = {5, 2, 1, 6};
    const int x = (portrait ? (c.width() - 23) / 2 : 50) + xOffset, y = portrait ? 78 : 35;
    for (unsigned i = 0; i < 4; ++i) {
      letter(c, core[i], x + i * 6, y, 1);
    }
  }
  const int barInset = portrait ? 8 : 16;
  const int barX = barInset + xOffset, barY = portrait ? c.height() - 22 : c.height() - 13;
  const int barWidth = c.width() - 2 * barInset;
  rect(c, barX, barY, barWidth, 5, false);
  const uint32_t fillMs = DurationMs - 200;
  const uint32_t t = elapsed > fillMs ? fillMs : elapsed;
  const uint32_t q = t * 1000 / fillMs;
  const uint32_t eased = q * q * (3000 - 2 * q) / 1000000;
  const int filled = eased * (barWidth - 4) / 1000;
  if (filled) {
    rect(c, barX + 2, barY + 2, filled, 1, true);
  }
}
// Unsigned subtraction supports the millis() wrap boundary.
struct Sequence {
  uint32_t bootAt = 0, startedAt = 0;
  bool consumed = false, started = false;
  void reset(uint32_t now) {
    bootAt = now;
    startedAt = 0;
    consumed = started = false;
  }
  void found(uint32_t now) {
    if (consumed) {
      return;
    }
    consumed = true;
    if (uint32_t(now - bootAt) < StartWindowMs) {
      startedAt = now;
      started = true;
    }
  }
  void dismiss() {
    consumed = true;
    started = false;
  }
  bool active(uint32_t now) const {
    return started && uint32_t(now - startedAt) < DurationMs;
  }
  uint32_t elapsed(uint32_t now) const {
    return uint32_t(now - startedAt);
  }
};
}  // namespace founderBoot
