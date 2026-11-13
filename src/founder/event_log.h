#pragma once
#include <Arduino.h>

#include "founder/automation_policy.h"
namespace founderEvents {
void begin();
void configuration(uint32_t revision);
void operation(bool paused);
void observe(unsigned device, const char* name, const founderAutomation::Config& config,
             bool enabled, bool canRun);
void appendJson(String& out);
}  // namespace founderEvents
