#pragma once
#include <Arduino.h>
namespace founderOutputs {
void preinit();
void begin();
void interlock(bool rest, bool setup);
void inhibit();
void stopAll();
bool assigned(unsigned channel);
bool startTest(unsigned channel, unsigned percent, unsigned durationMs);
bool setTestLevel(unsigned percent);
void stopTest();
bool testing();
bool inhibited();
bool ready();
bool set(const char* name, float percent);  // Explicit unassigned channel test only.
void assignOwners(const int8_t (&owners)[5], const bool (&timed)[5]);
bool setDevice(unsigned owner, unsigned channel, float percent);
bool pulseDevice(unsigned owner, unsigned channel, unsigned durationMs);
float applied(unsigned channel);
unsigned remainingMs(unsigned channel);
void appendJson(String& json);
}  // namespace founderOutputs
