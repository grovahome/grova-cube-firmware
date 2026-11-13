#pragma once
#include <Arduino.h>

#include "founder/automation_policy.h"
#include "modules/signal_registry.h"
namespace founderDevices {
founderAutomation::Reading readSignal(SignalId signal, const char* source);
using MeasurementVisitor = void (*)(const char*, SignalId, float, bool);
void forEachSignal(MeasurementVisitor visitor);
void begin();
void loop();
void appendJson(String& json);
}  // namespace founderDevices
