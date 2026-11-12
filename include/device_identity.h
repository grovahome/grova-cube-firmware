#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Arduino fills the uint64_t byte buffer in MAC wire order on little-endian ESP32.
// Use all six bytes: the low 24 bits alone can be the common manufacturer prefix.
inline void grovaMacSuffix(uint64_t mac, char (&out)[13]) {
  snprintf(out, sizeof(out), "%02X%02X%02X%02X%02X%02X", unsigned(mac & 0xff),
           unsigned((mac >> 8) & 0xff), unsigned((mac >> 16) & 0xff), unsigned((mac >> 24) & 0xff),
           unsigned((mac >> 32) & 0xff), unsigned((mac >> 40) & 0xff));
}
