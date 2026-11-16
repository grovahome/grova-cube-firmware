#pragma once
using SemaphoreHandle_t = int*;
inline int state = 0;
inline SemaphoreHandle_t xSemaphoreCreateMutex() {
  return &state;
}
inline int xSemaphoreTake(SemaphoreHandle_t s, int) {
  if (*s) {
    return 0;
  }
  *s = 1;
  return 1;
}
inline void xSemaphoreGive(SemaphoreHandle_t s) {
  *s = 0;
}
