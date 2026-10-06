#pragma once
#include <cstddef>
#include <cstdint>
inline void esp_fill_random(void* output, size_t count) {
  static uint8_t value = 0;
  for (size_t i = 0; i < count; ++i) static_cast<uint8_t*>(output)[i] = value++;
}
