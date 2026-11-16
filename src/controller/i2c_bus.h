#pragma once
#include <Arduino.h>
namespace founderBus {
void begin();
void refresh();
bool hasMux();
// All drivers own a scope for the entire transaction or library call.
// -1 = direct system bus, 0..7 = one isolated TCA9548A port.
class Scope {
 public:
  explicit Scope(int channel = -1);
  ~Scope();
  explicit operator bool() const {
    return ready_;
  }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;

 private:
  bool locked_ = false, ready_ = false;
};
bool probe(uint8_t address);
}  // namespace founderBus
