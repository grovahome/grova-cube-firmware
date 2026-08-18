#pragma once

#include "modules/fan_control.h"

struct FanDemandState {
  FanRuleState curveRules;
};

FanDemand fanDemand_evaluate(
  int fan,
  float temperatureTarget,
  float humidityTarget,
  FanDemandState& state
);
