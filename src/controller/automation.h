#pragma once
#include <ArduinoJson.h>

#include "controller/automation_policy.h"
namespace founderAutomation {
constexpr unsigned ConfigMaxBytes = 12288;
bool parse(JsonVariantConst value, Config& next, bool analog, bool pulse, unsigned maxRuntime,
           String& error);
void append(JsonObject row, const Config& config);
void appendDecision(JsonObject row, const Decision& decision, const Config& config,
                    const char* block = nullptr);
Reading read(unsigned signal, const char* source);
}  // namespace founderAutomation
