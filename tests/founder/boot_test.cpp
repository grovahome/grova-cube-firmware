#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include "founder/boot_animation.h"
struct Canvas {
  int w, h;
  std::vector<unsigned char> bits;
  Canvas(int width, int height) : w(width), h(height), bits(width * height) {}
  int width() const {
    return w;
  }
  int height() const {
    return h;
  }
  void drawPixel(int x, int y, int value) {
    assert(x >= 0 && y >= 0 && x < w && y < h);
    bits[y * w + x] = value ? 1 : 0;
  }
  unsigned count() const {
    unsigned n = 0;
    for (auto b : bits) {
      n += b;
    }
    return n;
  }
};
int main(int argc, char** argv) {
  founderBoot::Sequence s;
  s.reset(100);
  assert(!s.active(100));
  s.found(200);
  assert(s.active(200));
  assert(s.active(2399));
  assert(!s.active(2400));
  s.found(2500);
  assert(!s.active(2500));
  s.reset(0);
  s.found(100);
  s.dismiss();
  assert(!s.active(200));
  s.found(200);
  assert(!s.active(200));
  s.reset(0);
  s.found(10000);
  assert(!s.active(10000));  // No welcome on a late OLED hot-plug.
  s.reset(0xFFFFFF00U);
  s.found(0xFFFFFF80U);
  assert(s.active(200));
  assert(s.elapsed(200) == 328);
  assert(!s.active(2300));
  std::ofstream output;
  if (argc > 1) {
    output.open(argv[1]);
    assert(output);
    output << "[";
  }
  for (unsigned orientation = 0; orientation < 2; ++orientation) {
    const int w = orientation ? 64 : 128, h = orientation ? 128 : 64;
    unsigned previous = 0;
    if (output.is_open()) {
      if (orientation) {
        output << ",";
      }
      output << "{\"width\":" << w << ",\"height\":" << h << ",\"frames\":[";
    }
    for (uint32_t time = 0; time <= 2200; time += 100) {
      Canvas c(w, h);
      founderBoot::draw(c, time);
      assert(c.count() >= previous);
      previous = c.count();
      if (time >= 2000) {
        const int x = orientation ? 8 : 16, y = h - (orientation ? 22 : 13);
        for (int xx = x + 2; xx < w - x - 2; ++xx) {
          assert(c.bits[(y + 2) * w + xx]);
        }
      }
      if (output.is_open()) {
        if (time) {
          output << ",";
        }
        output << "\"";
        for (auto b : c.bits) {
          output << char(b ? '1' : '0');
        }
        output << "\"";
      }
    }
    assert(previous > 250);
    if (output.is_open()) {
      output << "]}";
    }
  }
  if (output.is_open()) {
    output << "]";
  }
  std::cout << "Boot graphics: bounds, progressive frames, complete bar, portrait/landscape, "
               "dismissal, late hot-plug and clock wrap passed\n";
}
