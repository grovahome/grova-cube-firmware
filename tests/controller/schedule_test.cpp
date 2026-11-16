#include <cassert>
#include <iostream>

#include "controller/schedule_policy.h"
using namespace founderSchedule;
int main() {
  Plan p;
  p.enabled = true;
  p.count = 2;
  p.slots[0] = {480, 600, 0, 100};
  p.slots[1] = {720, 840, 0, 50};
  assert(valid(p, false, 0));
  assert(percent(p, 479) == 0);
  assert(percent(p, 480) == 100);
  assert(percent(p, 599) == 100 && percent(p, 600) == 0 && percent(p, 720) == 50 &&
         percent(p, 840) == 0);
  p.slots[1].start = 590;
  assert(!valid(p, false, 0));
  p.count = 1;
  p.slots[0] = {1320, 360, 0, 100};
  assert(valid(p, false, 0));
  assert(percent(p, 0) == 100 && percent(p, 359) == 100 && percent(p, 360) == 0 &&
         percent(p, 1320) == 100);
  p.slots[0] = {0, 1440, 0, 100};
  assert(valid(p, false, 0) && percent(p, 1439) == 100);
  p.slots[0].end = 0;
  assert(!valid(p, false, 0));
  p.slots[0] = {480, 0, 1000, 100};
  assert(valid(p, true, 1000));
  assert(!valid(p, true, 999));
  p.count = 2;
  p.slots[1] = p.slots[0];
  assert(!valid(p, true, 1000));
  p.count = 0;
  assert(!valid(p, true, 1000));
  p.enabled = false;
  assert(valid(p, true, 1000));
  assert(token(2026001, 480) < token(2026001, 481));
  assert(token(2026001, 1439) < token(2026002, 0));
  Plan catchup;
  catchup.enabled = true;
  catchup.count = 2;
  catchup.catchUpMinutes = 30;
  catchup.slots[0] = {480, 0, 500, 100};
  catchup.slots[1] = {490, 0, 500, 100};
  assert(valid(catchup, true, 500));
  assert(latest(catchup, 2026001, 500).slot == 1);
  assert(latest(catchup, 2026001, 500).ageMinutes == 10);
  assert(previousDate(2026000) == 2025364 && previousDate(2025000) == 2024365);
  assert(latest(catchup, 2026000, 0).token == token(2025364, 490));
  catchup.catchUpMinutes = 61;
  assert(!valid(catchup, true, 500));
  Plan interval;
  interval.enabled = true;
  interval.interval.enabled = true;
  interval.interval.everyMinutes = 120;
  interval.interval.durationMinutes = 10;
  interval.interval.percent = 40;
  assert(valid(interval, false, 0));
  assert(percent(interval, 0) == 40 && percent(interval, 9) == 40 && percent(interval, 10) == 0 &&
         percent(interval, 120) == 40);
  assert(latest(interval, 2026001, 130).slot == 1 &&
         latest(interval, 2026001, 130).ageMinutes == 10);
  interval.interval.anchor = 1435;
  assert(percent(interval, 0) == 40 && percent(interval, 5) == 0);
  assert(latest(interval, 2026001, 0).token == token(2026000, 1435));
  interval.interval.runtimeMs = 500;
  assert(valid(interval, true, 500));
  assert(!valid(interval, true, 499));
  interval.interval.everyMinutes = 0;
  assert(!valid(interval, false, 0));
  interval.interval.everyMinutes = 120;
  interval.interval.durationMinutes = 121;
  assert(!valid(interval, false, 0));
  interval.interval.durationMinutes = 10;
  interval.count = 1;
  assert(!valid(interval, false, 0));
  std::cout << "Daily plan windows, midnight, overlap, boundaries and pulse limits passed\n";
}
