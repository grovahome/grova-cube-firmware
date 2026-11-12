#pragma once
#include <stdint.h>
namespace founderDigital {
constexpr unsigned MaxRuntimeMs = 10000;
bool begin();
bool ready();
void set(unsigned channel, bool on);
bool pulse(unsigned channel, unsigned durationMs);
bool on(unsigned channel);
unsigned remainingMs(unsigned channel);
}  // namespace founderDigital
