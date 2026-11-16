#include <cassert>
#include <cstring>
#include <iostream>

#include "founder/event_log_policy.h"
using namespace founderEvents;
int main() {
  Recorder log;
  log.begin(0, 0);
  assert(log.count == 1 && log.newest(0).epoch == 0);
  log.operation(true, 1, 0);
  log.operation(true, 2, 0);
  assert(log.count == 2);
  founderAutomation::Config c;
  c.count = 1;
  strcpy(c.rules[0].source, "sensor@ch0");
  c.rules[0].target = 24;
  c.rules[0].hasMaximum = true;
  c.rules[0].maximum = 30;
  c.rules[0].hysteresis = 1;
  c.rules[0].delaySeconds = 2;
  c.fallbackPercent = 35;
  float value = 32;
  bool valid = true;
  uint32_t age = 0;
  auto read = [&](unsigned, const char*) { return founderAutomation::Reading(value, valid, age); };
  log.observe(0, "Climate", c, true, true, 10, 100, read);
  assert(log.count == 2);
  log.observe(0, "Climate", c, true, true, 2010, 102, read);
  assert(log.newest(0).type == Type::Maximum && log.newest(0).value == 32);
  assert(log.newest(0).hasLimit && log.newest(0).limit == 30);
  c.rules[0].source[0] = 'X';
  c.rules[0].maximum = 40;
  assert(!strcmp(log.newest(0).source, "sensor@ch0") && log.newest(0).limit == 30);
  c.rules[0].source[0] = 's';
  c.rules[0].maximum = 30;
  auto count = log.count;
  for (unsigned n = 0; n < 1000; ++n) {
    log.observe(0, "Climate", c, true, true, 2011 + n, 103, read);
  }
  assert(log.count == count);
  value = 29.5f;
  log.observe(0, "Climate", c, true, true, 4000, 104, read);
  assert(log.count == count);
  value = 28;
  log.observe(0, "Climate", c, true, true, 5000, 105, read);
  log.observe(0, "Climate", c, true, true, 7000, 107, read);
  assert(log.newest(0).type == Type::LimitCleared);
  valid = false;
  log.observe(0, "Climate", c, true, false, 8000, 108, read);
  assert(log.newest(0).type == Type::FallbackRequired && log.newest(1).type == Type::InputLost);
  count = log.count;
  for (unsigned n = 0; n < 100; ++n) {
    log.observe(0, "Climate", c, true, false, 8001 + n, 108, read);
  }
  assert(log.count == count);
  log.observe(0, "Climate", c, true, true, 8200, 108, read);
  assert(log.newest(0).type == Type::FallbackActive);
  valid = true;
  log.observe(0, "Climate", c, true, true, 9000, 109, read);
  assert(log.newest(0).type == Type::FallbackEnded && log.newest(1).type == Type::InputRecovered);
  count = log.count;
  age = 9999;
  log.observe(0, "Climate", c, true, true, 9999, 109, read);
  assert(log.count == count);
  age = 10000;
  log.observe(0, "Climate", c, true, true, 10000, 110, read);
  assert(log.newest(1).inputState == 2);
  log.configuration(2, 11000, 111);
  assert(log.newest(0).type == Type::ConfigurationSaved && log.newest(0).revision == 2);
  for (unsigned n = 0; n < Capacity + 10; ++n) {
    log.add(Type::Paused, n, 0);
  }
  assert(log.count == Capacity && log.newest(0).sequence == log.total &&
         log.newest(Capacity - 1).sequence == log.total - Capacity + 1);
  log.begin(UINT32_MAX - 1000, 0);
  age = 0;
  value = 32;
  log.observe(0, "Climate", c, true, true, UINT32_MAX - 1000, 0, read);
  log.observe(0, "Climate", c, true, true, 1000, 0, read);
  assert(log.newest(0).type == Type::Maximum);
  log.begin(0, 0);
  assert(log.count == 1 && log.total == 1);
  std::cout << "RAM events: bounded overwrite, deduplication, missing/recovery, "
               "limits/hysteresis/debounce, fallback, restart and millis wrap passed\n";
}
