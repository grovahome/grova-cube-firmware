#pragma once
#include <stdint.h>
namespace founderRtc {
bool begin();
bool read(uint32_t& utc);
bool write(uint32_t utc);
bool present();
bool backupConfigured();
}  // namespace founderRtc
