#include <cassert>
#include <iostream>

#include "founder/schedule_policy.h"
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
  std::cout << "Daily plan windows, midnight, overlap, boundaries and pulse limits passed\n";
}
