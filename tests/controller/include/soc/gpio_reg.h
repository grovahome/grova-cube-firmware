#pragma once
#include <Arduino.h>
#define GPIO_OUT_W1TS_REG 1
#define GPIO_OUT_W1TC_REG 0
inline void testRegWrite(int reg, uint32_t mask) {
  for (int i = 0; i < 32; ++i) {
    if (mask & (1UL << i)) {
      digitalWrite(i, reg);
    }
  }
}
#define REG_WRITE(reg, mask) testRegWrite(reg, mask)
