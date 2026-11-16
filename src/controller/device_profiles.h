#pragma once
#include <stdint.h>
#include <string.h>

#include "boards/grova_core_s3.h"
namespace founderEquipment {
constexpr unsigned Count = 5;
enum class Kind : uint8_t {
  None,
  Switch,
  Pump,
  TimedValve,
  SwitchedLight,
  DimmableLight,
  SwitchedFan,
  AnalogFan
};
struct Profile {
  Kind kind;
  const char* id;
  const char* label;
  bool analog;
  bool timed;
  bool optionalPower;
  unsigned maxRuntimeMs;
  unsigned startupLockMs;
};
inline const Profile* profiles(unsigned& count) {
  static const Profile all[] = {
      {Kind::Switch, "switch", "Allgemeiner Schaltverbraucher", false, false, false, 0, 0},
      {Kind::Pump, "pump", "DC-Pumpe", false, true, false, 10000, 600000},
      {Kind::TimedValve, "valve_pulse", "Monostabiles Ventil (stromlos geschlossen)", false, true,
       false, 10000, 0},
      {Kind::SwitchedLight, "light_switch", "DC-Beleuchtung (Ein/Aus)", false, false, false, 0, 0},
      {Kind::DimmableLight, "light_0_10v", "Beleuchtung mit 0-10-V-Steuereingang", true, false,
       true, 0, 0},
      {Kind::SwitchedFan, "fan_switch", "DC-Luefter (Ein/Aus, ohne Tacho)", false, false, false, 0,
       0},
      {Kind::AnalogFan, "fan_0_10v", "Luefter mit 0-10-V-Steuereingang", true, false, false, 0, 0}};
  count = sizeof(all) / sizeof(all[0]);
  return all;
}
inline const Profile* profile(const char* id) {
  unsigned n;
  const auto* all = profiles(n);
  for (unsigned i = 0; i < n; ++i) {
    if (!strcmp(id, all[i].id)) {
      return all + i;
    }
  }
  return nullptr;
}
inline const founder::OutputSpec* channels() {
  return founder::OUTPUTS;
}
inline int channelIndex(const char* id) {
  for (int i = 0; i < 5; ++i) {
    if (!strcmp(channels()[i].id, id)) {
      return i;
    }
  }
  return -1;
}
}  // namespace founderEquipment
