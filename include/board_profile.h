#pragma once

// The active firmware line currently targets only the existing GROVA PCB v1.
// When the Founder Edition hardware is available, its firmware branch can
// replace this include with its own board profile while keeping shared modules
// independent from the physical pin map.
#include "boards/grova_core_v1.h"
