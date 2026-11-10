#pragma once
#include <math.h>
namespace founderOutputs {
struct Policy {
  bool ready = false, rest = true, setup = true, ota = false;
  bool allows(float value) const {
    return ready && isfinite(value) && value >= 0 && value <= 100 &&
           (value == 0 || (!rest && !setup && !ota));
  }
};
}  // namespace founderOutputs
