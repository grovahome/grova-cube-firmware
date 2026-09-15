#pragma once

void pump_begin();
// Starts only when the independent cutoff is ready. Rejects invalid durations.
bool pump_startTimed(unsigned long runtimeMs);
bool pump_isRunning();
// OTA interlock: stop now and reject further starts until reboot.
void pump_inhibit();
void pump_off();
