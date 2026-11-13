#include <cassert>
#include <cstring>
#include <iostream>

#include "founder/automation_policy.h"
using namespace founderAutomation;
int main() {
  Config c;
  c.count = 2;
  strcpy(c.rules[0].source, "temp@ch0");
  strcpy(c.rules[1].source, "hum@ch1");
  c.rules[0].target = 24;
  c.rules[0].band = 4;
  c.rules[0].hasMaximum = true;
  c.rules[0].maximum = 30;
  c.rules[0].abovePercent = 0;
  c.rules[1].signal = 1;
  c.rules[1].target = 65;
  c.rules[1].band = 10;
  c.rules[1].hasMaximum = true;
  c.rules[1].maximum = 75;
  c.rules[1].abovePercent = 100;
  c.rules[0].delaySeconds = c.rules[1].delaySeconds = 2;
  float temp = 26, hum = 70;
  bool missing = false;
  uint32_t age = 0;
  auto read = [&](unsigned signal, const char*) {
    return Reading{signal ? hum : temp, !missing, age};
  };
  State s;
  assert(valid(c, true, false, 0));
  auto d = evaluate(c, s, 0, true, read);
  assert(d.percent == 0);
  d = evaluate(c, s, 1999, true, read);
  assert(d.percent == 0);
  d = evaluate(c, s, 2000, true, read);
  assert(d.percent == 50 && d.winner == 0);
  assert(d.evaluated && d.evaluatedAt == 2000 && d.rules[0].input.value == 26 &&
         d.rules[1].normalPercent == 50);
  hum = 74;
  d = evaluate(c, s, 2001, true, read);
  assert(d.percent == 90 && d.winner == 1);
  temp = 35;
  hum = 80;
  evaluate(c, s, 3000, true, read);
  d = evaluate(c, s, 5000, true, read);
  assert(d.percent == 0 && d.winner == 0 && d.warnings == 10 && !strcmp(d.reason, "ABOVE_MAXIMUM"));
  assert(d.rules[0].requestedPercent == 0 && d.rules[1].requestedPercent == 100 &&
         d.rules[1].action == 100);
  c.rules[0].abovePercent = -1;
  d = evaluate(c, s, 5001, true, read);
  assert(d.percent == 100 && d.winner == 1);
  c.fallbackPercent = 40;
  missing = true;
  d = evaluate(c, s, 5002, true, read);
  assert(d.percent == 40 && !strcmp(d.reason, "SENSOR_MISSING"));
  assert(d.missingMask == 3 && !d.rules[0].input.valid && !d.rules[1].input.valid);
  missing = false;
  age = 12000;
  assert(evaluate(c, s, 5003, true, read).percent == 40);
  age = 0;
  c.count = 1;
  c.rules[0].hasMinimum = true;
  c.rules[0].minimum = 18;
  c.rules[0].belowPercent = 0;
  c.rules[0].delaySeconds = 0;
  temp = 17;
  d = evaluate(c, s, 6000, true, read);
  assert(d.percent == 0 && d.warnings == 1);
  temp = 18.2f;
  d = evaluate(c, s, 6001, true, read);
  assert(d.warnings == 1);
  temp = 19;
  assert(evaluate(c, s, 6002, true, read).warnings == 0);
  c.rules[0].raise = true;
  temp = 20;
  assert(evaluate(c, s, 6003, true, read).percent == 100);
  c.fallbackPercent = 0;
  assert(valid(c, false, true, 1000));
  c.fallbackPercent = 100;
  assert(!valid(c, false, true, 1000));
  c.fallbackPercent = 0;
  c.rules[0].minimum = 25;
  assert(!valid(c, true, false, 0));
  c.rules[0].minimum = 18;
  c.rules[0].delaySeconds = 2;
  s = {};
  temp = 20;
  evaluate(c, s, UINT32_MAX - 1000, true, read);
  assert(evaluate(c, s, 1000, true, read).percent == 100);
  std::cout << "Sensor control: target, combined demand, priority, warnings, hysteresis, debounce, "
               "missing/stale fallback and wrap passed\n";
}
