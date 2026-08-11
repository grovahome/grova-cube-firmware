#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "modules/fan_control.h"

static constexpr int AUTO_CURVE_POINT_COUNT = 10;

// Code-only defaults. The active climate targets still come from the current
// day/night or local grow configuration.
static const FanControlConfig FAN1_CONFIG = {
  FAN_MODE_AUTOMATIC,
  50,
  0,
  FAN_IDLE_PERCENT,
  FAN_MAX_PERCENT,
  60,
  1500UL,
  60000UL,
  FAN_RAMP_UP_PWM_STEP,
  FAN_RAMP_DOWN_PWM_STEP,
  FAN_RAMP_INTERVAL_MS,
  {FAN_TACHO_MIN_CHECK_PERCENT, FAN_TACHO_MIN_RPM, FAN_TACHO_FAULT_DELAY_MS},
  {true, 1.5F, 7.0F, 0.5F, FAN_CURVE_NORMAL},
  {true, 5.0F, 20.0F, 3.0F, FAN_CURVE_NORMAL}
};

static const FanControlConfig FAN2_CONFIG = {
  FAN_MODE_MANUAL,
  0,
  0,
  FAN_IDLE_PERCENT,
  FAN_MAX_PERCENT,
  60,
  1500UL,
  60000UL,
  FAN_RAMP_UP_PWM_STEP,
  FAN_RAMP_DOWN_PWM_STEP,
  FAN_RAMP_INTERVAL_MS,
  {FAN_TACHO_MIN_CHECK_PERCENT, FAN_TACHO_MIN_RPM, FAN_TACHO_FAULT_DELAY_MS},
  {false, 1.5F, 7.0F, 0.5F, FAN_CURVE_NORMAL},
  {false, 5.0F, 20.0F, 3.0F, FAN_CURVE_NORMAL}
};

static int clampPercent(int value, int minimum, int maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}

static float curveExponent(FanCurveStyle curve) {
  if (curve == FAN_CURVE_GENTLE) return 2.2F;
  if (curve == FAN_CURVE_AGGRESSIVE) return 0.65F;
  return 1.35F;
}

static int evaluateCurve(
  const FanSourceRuleConfig& rule,
  float value,
  float target,
  int minimumPercent,
  int maximumPercent,
  bool& active
) {
  if (!rule.enabled || rule.fullLoadAboveTarget <= 0.0F) {
    active = false;
    return 0;
  }

  const float start = target - max(0.0F, rule.leadBeforeTarget);
  const float stop = start - max(0.0F, rule.hysteresis);
  if (active) {
    if (value < stop) active = false;
  } else if (value >= start) {
    active = true;
  }

  if (!active) return 0;

  const float fullLoad = target + rule.fullLoadAboveTarget;
  if (value >= fullLoad) return maximumPercent;
  if (value <= start) return minimumPercent;

  const float progress = (value - start) / (fullLoad - start);
  const float exponent = curveExponent(rule.curve);

  // Generate ten deterministic curve points, then interpolate between the
  // surrounding pair. Only the compact rule parameters are stored in code.
  float pointValue[AUTO_CURVE_POINT_COUNT];
  float pointPercent[AUTO_CURVE_POINT_COUNT];
  for (int i = 0; i < AUTO_CURVE_POINT_COUNT; i++) {
    const float pointProgress = static_cast<float>(i) / (AUTO_CURVE_POINT_COUNT - 1);
    pointValue[i] = start + pointProgress * (fullLoad - start);
    pointPercent[i] = minimumPercent +
      (maximumPercent - minimumPercent) * powf(pointProgress, exponent);
  }

  for (int i = 1; i < AUTO_CURVE_POINT_COUNT; i++) {
    if (value <= pointValue[i]) {
      const float segment = (value - pointValue[i - 1]) / (pointValue[i] - pointValue[i - 1]);
      const float output = pointPercent[i - 1] + segment * (pointPercent[i] - pointPercent[i - 1]);
      return clampPercent(lroundf(output), minimumPercent, maximumPercent);
    }
  }

  return maximumPercent;
}

const FanControlConfig& fanControl_getConfig(int fan) {
  return fan == 2 ? FAN2_CONFIG : FAN1_CONFIG;
}

const char* fanControl_modeName(FanOperatingMode mode) {
  if (mode == FAN_MODE_OFF) return "OFF";
  if (mode == FAN_MODE_MANUAL) return "MANUAL";
  return "AUTO";
}

const char* fanControl_curveName(FanCurveStyle curve) {
  if (curve == FAN_CURVE_GENTLE) return "GENTLE";
  if (curve == FAN_CURVE_AGGRESSIVE) return "AGGRESSIVE";
  return "NORMAL";
}

FanDemand fanControl_evaluate(
  int fan,
  float temperature,
  float humidity,
  float temperatureTarget,
  float humidityTarget,
  FanRuleState& state
) {
  const FanControlConfig& config = fanControl_getConfig(fan);
  FanDemand demand;
  demand.temperaturePercent = evaluateCurve(
    config.temperature,
    temperature,
    temperatureTarget,
    config.minimumPercent,
    config.maximumPercent,
    state.temperatureActive
  );
  demand.humidityPercent = evaluateCurve(
    config.humidity,
    humidity,
    humidityTarget,
    config.minimumPercent,
    config.maximumPercent,
    state.humidityActive
  );
  demand.percent = max(config.basePercent, max(demand.temperaturePercent, demand.humidityPercent));

  if (demand.temperaturePercent > 0 && demand.humidityPercent > 0) demand.reason = "TEMP+HUM";
  else if (demand.temperaturePercent > 0) demand.reason = "TEMP";
  else if (demand.humidityPercent > 0) demand.reason = "HUM";
  else if (config.basePercent > 0) demand.reason = "BASE";
  else demand.reason = "IDLE";

  return demand;
}

static void appendSourceConfig(String& json, const char* source, const FanSourceRuleConfig& rule) {
  json += "\"";
  json += source;
  json += "\":{";
  json += "\"enabled\":";
  json += rule.enabled ? "true" : "false";
  json += ",\"lead_before_target\":";
  json += String(rule.leadBeforeTarget, 1);
  json += ",\"full_load_above_target\":";
  json += String(rule.fullLoadAboveTarget, 1);
  json += ",\"hysteresis\":";
  json += String(rule.hysteresis, 1);
  json += ",\"curve\":\"";
  json += fanControl_curveName(rule.curve);
  json += "\"}";
}

static void appendFanConfig(String& json, int fan, const FanControlConfig& config) {
  json += "\"fan";
  json += fan;
  json += "\":{";
  json += "\"default_mode\":\"";
  json += fanControl_modeName(config.defaultMode);
  json += "\",\"manual_percent\":";
  json += config.manualPercent;
  json += ",\"base_percent\":";
  json += config.basePercent;
  json += ",\"minimum_percent\":";
  json += config.minimumPercent;
  json += ",\"maximum_percent\":";
  json += config.maximumPercent;
  json += ",\"startup_boost_percent\":";
  json += config.startupBoostPercent;
  json += ",\"startup_boost_ms\":";
  json += config.startupBoostMs;
  json += ",\"minimum_run_ms\":";
  json += config.minimumRunMs;
  json += ",\"combine\":\"MAXIMUM\",";
  appendSourceConfig(json, "temperature", config.temperature);
  json += ",";
  appendSourceConfig(json, "humidity", config.humidity);
  json += "}";
}

void fanControl_appendJson(String& json) {
  json += "\"fan_control\":{";
  json += "\"curve_points\":";
  json += AUTO_CURVE_POINT_COUNT;
  json += ",\"sensor_failure_percent\":";
  json += FAN_SENSOR_FAIL_PERCENT;
  json += ",";
  appendFanConfig(json, 1, FAN1_CONFIG);
  json += ",";
  appendFanConfig(json, 2, FAN2_CONFIG);
  json += "}";
}
