#pragma once
#include <Arduino.h>
namespace founderOutputs {
void preinit();
void begin();
void interlock(bool rest, bool setup);
void inhibit();
bool inhibited();
bool ready();
bool set(const char* name, float percent);
void appendJson(String& json);
}  // namespace founderOutputs
