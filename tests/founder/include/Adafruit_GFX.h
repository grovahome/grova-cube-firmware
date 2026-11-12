#pragma once
#include <algorithm>
#include <array>
class GFXcanvas1 {
  std::array<uint8_t, 1024> pixels{};

 public:
  GFXcanvas1(int, int) {}
  uint8_t* getBuffer() { return pixels.data(); }
  void fillScreen(int value) { pixels.fill(value ? 255 : 0); }
};
