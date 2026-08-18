#include <Arduino.h>
#include "modules/fan_control.h"
#include "modules/fan_demand.h"
#include "modules/fan_interval.h"
#include "modules/fan_schedule.h"

FanDemand fanDemand_evaluate(
  int fan,
  float temperatureTarget,
  float humidityTarget,
  FanDemandState& state
) {
  FanDemand demand = fanControl_evaluateCurves(
    fan, temperatureTarget, humidityTarget, state.curveRules);
  const FanAutomationConfig& config = fanControl_getAutomationConfig(fan);
  const FanIntervalDemand interval = fanInterval_evaluate(fan);
  const FanScheduleDemand schedule = fanSchedule_evaluate(fan);

  demand.curvePercent = demand.percent;
  demand.basePercent = max(0, config.basePercent);
  demand.intervalPercent = interval.percent;
  demand.schedulePercent = schedule.percent;
  demand.percent = 0;
  demand.reason = "IDLE";
  demand.winningProducer = "NONE";

  if (demand.basePercent > 0) {
    demand.percent = demand.basePercent;
    demand.reason = "BASE";
    demand.winningProducer = "BASE";
  }
  if (demand.curvePercent >= demand.percent && demand.curvePercent > 0) {
    demand.percent = demand.curvePercent;
    demand.reason = demand.curveReason;
    demand.winningProducer = "CURVE";
  }
  if (demand.intervalPercent > demand.percent) {
    demand.percent = demand.intervalPercent;
    demand.reason = interval.reason;
    demand.winningProducer = "INTERVAL";
  }
  if (demand.schedulePercent > demand.percent) {
    demand.percent = demand.schedulePercent;
    demand.reason = schedule.reason;
    demand.winningProducer = "SCHEDULE";
  }

  return demand;
}
