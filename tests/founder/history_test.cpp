#include <cassert>
#include <cmath>
#include <iostream>

#include "founder/history_policy.h"
using namespace founderHistory;
int main() {
  Recorder h;
  h.begin();
  assert(h.start(100, 0));
  h.put("sensor-a", 0, 24);
  assert(!h.start(30100 - 1, 0));
  assert(h.start(30100, 100));
  h.put("sensor-b", 0, 26);
  assert(h.count == 2 && h.series[0].source[7] == 'a' && std::isnan(h.series[0].values[h.current]));
  for (unsigned n = 2; n < Capacity + 3; ++n) {
    assert(h.start(100 + n * IntervalMs, 100 + n));
    h.put("sensor-b", 0, 20 + n);
  }
  assert(h.count == Capacity && !h.series[0].used);
  assert(h.stamps[h.position(0)].uptime == 100 + 3 * IntervalMs);
  h.begin();
  assert(h.start(UINT32_MAX - 10000, 0));
  assert(!h.start(10000, 0));
  assert(h.start(20000, 0));
  h.begin();
  assert(h.start(0, 0));
  for (unsigned n = 0; n < MaxSeries; ++n) {
    assert(h.put("sensor", n, 1));
  }
  assert(!h.put("extra", 0, 1) && h.omitted == 1);
  h.begin();
  assert(h.count == 0 && !h.sampled);
  std::cout << "RAM history: sample interval, source isolation, gaps, bounded overwrite, prune, "
               "series limit, clock wrap and reboot passed\n";
}
