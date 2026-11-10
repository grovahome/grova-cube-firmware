#pragma once
#include <cmath>

#include "../../host/include/Arduino.h"
#define HIGH 1
#define LOW 0
#define OUTPUT 1
void digitalWrite(int, int);
void pinMode(int, int);
double ledcSetup(int, double, int);
void ledcAttachPin(int, int);
void ledcWrite(int, int);
void neopixelWrite(int, int, int, int);

struct TestESP {
  uint64_t getEfuseMac() {
    return 0x123456;
  }
  uint32_t getFlashChipSize() {
    return 16777216;
  }
  uint32_t getPsramSize() {
    return 8388608;
  }
  uint32_t getFreeHeap() {
    return 200000;
  }
};
inline TestESP ESP;

#define HEX 16
using std::isfinite;
using std::isnan;
void delay(unsigned long);
template <class T>
T constrain(T value, T low, T high) {
  return value < low ? low : value > high ? high : value;
}
