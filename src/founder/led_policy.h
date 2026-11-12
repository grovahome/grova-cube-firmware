#pragma once
#include <stdint.h>
namespace founderLed {
struct Color { uint8_t r, g, b; };
inline bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
struct State { bool error, setup, testing, connected; };
class Policy {
 public:
  void startTest(uint32_t now) { testing_ = true; started_ = now; }
  bool testActive(uint32_t now) const { return testing_ && uint32_t(now - started_) < 12000; }
  Color color(uint32_t now, State state) {
    if (testActive(now)) {
      const uint32_t elapsed = now - started_;
      if (elapsed < 3000) return {24, 0, 0};
      if (elapsed < 6000) return {0, 24, 0};
      if (elapsed < 9000) return {0, 0, 24};
      if (elapsed < 10500) return {16, 16, 16};
      return {0, 0, 0};
    }
    testing_ = false;
    const bool blink = (now % 1000) < 500;
    if (state.error) return blink ? Color{24, 0, 0} : Color{0, 0, 0};
    if (state.testing) return blink ? Color{20, 10, 0} : Color{0, 0, 0};
    if (state.setup) return blink ? Color{0, 0, 20} : Color{0, 0, 0};
    if (state.connected) return {0, 10, 0};
    return blink ? Color{20, 10, 0} : Color{0, 0, 0};
  }
 private:
  bool testing_ = false;
  uint32_t started_ = 0;
};
}  // namespace founderLed
