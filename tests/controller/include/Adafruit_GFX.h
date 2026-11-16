#pragma once
#include <stdint.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <string>
namespace testGraphics {
inline unsigned textCalls = 0;
inline std::string lines;
}  // namespace testGraphics
class GFXcanvas1 {
  std::array<uint8_t, 1024> pixels{};
  int w, h;
  unsigned rotation = 0;

 public:
  GFXcanvas1(int width, int height) : w(width), h(height) {
    assert(width * height <= 8192);
  }
  uint8_t* getBuffer() {
    return pixels.data();
  }
  void fillScreen(int value) {
    pixels.fill(value ? 255 : 0);
  }
  int width() const {
    return rotation & 1 ? h : w;
  }
  int height() const {
    return rotation & 1 ? w : h;
  }
  void setRotation(unsigned r) {
    rotation = r & 3;
  }
  void drawPixel(int x, int y, int value) {
    assert(x >= 0 && y >= 0 && x < width() && y < height());
    int t;
    if (rotation == 1) {
      t = x;
      x = w - 1 - y;
      y = t;
    } else if (rotation == 2) {
      x = w - 1 - x;
      y = h - 1 - y;
    } else if (rotation == 3) {
      t = x;
      x = y;
      y = h - 1 - t;
    }
    auto& b = pixels[y * ((w + 7) / 8) + x / 8];
    const uint8_t mask = 0x80 >> (x & 7);
    if (value) {
      b |= mask;
    } else {
      b &= ~mask;
    }
  }
  void setTextSize(int) {}
  void setTextColor(int) {}
  void setCursor(int, int) {}
  template <class T>
  void print(const T&) {
    ++testGraphics::textCalls;
  }
  void print(float, int) {
    ++testGraphics::textCalls;
  }
  template <class T>
  void println(const T&) {
    ++testGraphics::textCalls;
  }
  void println(const char* s) {
    ++testGraphics::textCalls;
    testGraphics::lines += s;
    testGraphics::lines += '\n';
  }
  void printf(const char*, ...) {
    ++testGraphics::textCalls;
  }
};
