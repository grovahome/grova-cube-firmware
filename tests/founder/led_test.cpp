#include "founder/led_policy.h"
#include <cassert>
#include <iostream>
using namespace founderLed;
int main() {
  Policy p; const State online{false,false,false,true};
  assert(same(p.color(0,online),{0,10,0}));
  assert(same(p.color(0,{true,true,false,true}),{24,0,0}));
  assert(same(p.color(750,{true,false,false,true}),{0,0,0}));
  assert(same(p.color(0,{false,true,false,false}),{0,0,20}));
  assert(same(p.color(0,{false,true,true,false}),{20,10,0}));
  p.startTest(100);
  assert(same(p.color(100,online),{24,0,0}));
  assert(same(p.color(3100,online),{0,24,0}));
  assert(same(p.color(6100,online),{0,0,24}));
  assert(same(p.color(9100,online),{16,16,16}));
  assert(same(p.color(10600,online),{0,0,0}));
  assert(same(p.color(12100,online),{0,10,0}));
  p.startTest(UINT32_MAX-1000);
  assert(same(p.color(1999,online),{0,24,0}));
  assert(!p.testActive(10999));
  std::cout << "LED status priority, RGB test sequence, expiry and clock wrap passed\n";
}
