#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "modules/fan_control.h"

static constexpr int AUTO_CURVE_POINT_COUNT = 10;

// Code-only defaults. The active climate targets still come from the current
// day/night or local grow configuration.
// Both physical fan channels use the same device profile. Differences between
// their jobs belong to the automation layer, not the hardware/device layer.
static const FanDeviceConfig STANDARD_FAN_DEVICE_CONFIG = {
  FAN_IDLE_PERCENT,
  FAN_MAX_PERCENT,
  60,
  1500UL,
  FAN_RAMP_UP_PWM_STEP,
  FAN_RAMP_DOWN_PWM_STEP,
  FAN_RAMP_INTERVAL_MS,
  {FAN_TACHO_MIN_CHECK_PERCENT, FAN_TACHO_MIN_RPM, FAN_TACHO_FAULT_DELAY_MS}
};

// Every fan has the same capabilities. Only policy data differs per channel.
static const FanAutomationConfig FAN_AUTOMATION_CONFIGS[FAN_CHANNEL_COUNT] = {
  {
    FAN_MODE_AUTOMATIC,
    50,
    0,
    60000UL,
    2,
    {
      {
        "TEMP", SIGNAL_CLIMATE_TEMPERATURE_C, true, FAN_RULE_ABOVE,
        FAN_TARGET_CLIMATE_TEMPERATURE, 0.0F, 1.5F, 7.0F, 0.5F, FAN_CURVE_NORMAL
      },
      {
        "HUM", SIGNAL_CLIMATE_HUMIDITY_PCT, true, FAN_RULE_ABOVE,
        FAN_TARGET_CLIMATE_HUMIDITY, 0.0F, 5.0F, 20.0F, 3.0F, FAN_CURVE_NORMAL
      }
    }
  },
  {
    FAN_MODE_MANUAL,
    0,
    0,
    60000UL,
    2,
    {
      {
        "TEMP", SIGNAL_CLIMATE_TEMPERATURE_C, false, FAN_RULE_ABOVE,
        FAN_TARGET_CLIMATE_TEMPERATURE, 0.0F, 1.5F, 7.0F, 0.5F, FAN_CURVE_NORMAL
      },
      {
        "HUM", SIGNAL_CLIMATE_HUMIDITY_PCT, false, FAN_RULE_ABOVE,
        FAN_TARGET_CLIMATE_HUMIDITY, 0.0F, 5.0F, 20.0F, 3.0F, FAN_CURVE_NORMAL
      }
    }
  }
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
  if (!rule.enabled || rule.fullLoadBeyondTarget <= 0.0F) {
    active = false;
    return 0;
  }

  const float lead = max(0.0F, rule.leadBeforeTarget);
  const float hysteresis = max(0.0F, rule.hysteresis);
  float progress = 0.0F;

  if (rule.direction == FAN_RULE_BELOW) {
    const float start = target + lead;
    const float stop = start + hysteresis;
    if (active) {
      if (value > stop) active = false;
    } else if (value <= start) {
      active = true;
    }
    if (!active) return 0;
    progress = (start - value) / (lead + rule.fullLoadBeyondTarget);
  } else {
    const float start = target - lead;
    const float stop = start - hysteresis;
    if (active) {
      if (value < stop) active = false;
    } else if (value >= start) {
      active = true;
    }
    if (!active) return 0;
    progress = (value - start) / (lead + rule.fullLoadBeyondTarget);
  }

  progress = constrain(progress, 0.0F, 1.0F);
  if (progress >= 1.0F) return maximumPercent;
  if (progress <= 0.0F) return minimumPercent;

  const float exponent = curveExponent(rule.curve);

  // Generate ten deterministic curve points, then interpolate between the
  // surrounding pair. Only the compact rule parameters are stored in code.
  float pointProgress[AUTO_CURVE_POINT_COUNT];
  float pointPercent[AUTO_CURVE_POINT_COUNT];
  for (int i = 0; i < AUTO_CURVE_POINT_COUNT; i++) {
    pointProgress[i] = static_cast<float>(i) / (AUTO_CURVE_POINT_COUNT - 1);
    pointPercent[i] = minimumPercent +
      (maximumPercent - minimumPercent) * powf(pointProgress[i], exponent);
  }

  for (int i = 1; i < AUTO_CURVE_POINT_COUNT; i++) {
    if (progress <= pointProgress[i]) {
      const float segment = (progress - pointProgress[i - 1]) /
        (pointProgress[i] - pointProgress[i - 1]);
      const float output = pointPercent[i - 1] + segment * (pointPercent[i] - pointPercent[i - 1]);
      return clampPercent(lroundf(output), minimumPercent, maximumPercent);
    }
  }

  return maximumPercent;
}

const FanDeviceConfig& fanControl_getDeviceConfig(int fan) {
  (void)fan;
  return STANDARD_FAN_DEVICE_CONFIG;
}

const FanAutomationConfig& fanControl_getAutomationConfig(int fan) {
  const int index = fan >= 1 && fan <= FAN_CHANNEL_COUNT ? fan - 1 : 0;
  return FAN_AUTOMATION_CONFIGS[index];
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

const char* fanControl_directionName(FanRuleDirection direction) {
  return direction == FAN_RULE_BELOW ? "BELOW" : "ABOVE";
}

const char* fanControl_targetSourceName(FanTargetSource source) {
  if (source == FAN_TARGET_CLIMATE_TEMPERATURE) return "CLIMATE_TEMPERATURE";
  if (source == FAN_TARGET_CLIMATE_HUMIDITY) return "CLIMATE_HUMIDITY";
  return "FIXED";
}

static float resolveTarget(
  const FanSourceRuleConfig& rule,
  float temperatureTarget,
  float humidityTarget
) {
  if (rule.targetSource == FAN_TARGET_CLIMATE_TEMPERATURE) return temperatureTarget;
  if (rule.targetSource == FAN_TARGET_CLIMATE_HUMIDITY) return humidityTarget;
  return rule.fixedTarget;
}

static bool applyLegacyFallback(SignalId source, float& value) {
  if (source == SIGNAL_CLIMATE_TEMPERATURE_C) {
    value = 22.0F;
    return true;
  }
  if (source == SIGNAL_CLIMATE_HUMIDITY_PCT) {
    value = 60.0F;
    return true;
  }
  return false;
}

FanDemand fanControl_evaluate(
  int fan,
  float temperatureTarget,
  float humidityTarget,
  FanRuleState& state
) {
  const FanDeviceConfig& deviceConfig = fanControl_getDeviceConfig(fan);
  const FanAutomationConfig& automationConfig = fanControl_getAutomationConfig(fan);
  FanDemand demand;
  demand.percent = automationConfig.basePercent;
  demand.ruleCount = constrain(
    automationConfig.ruleCount, 0, FAN_MAX_RULES_PER_CHANNEL);

  for (int i = 0; i < demand.ruleCount; i++) {
    const FanSourceRuleConfig& rule = automationConfig.rules[i];
    const SignalValue signal = signalRegistry_read(rule.source);
    float value = signal.value;
    const float target = resolveTarget(rule, temperatureTarget, humidityTarget);
    FanRuleDemand& ruleDemand = demand.rules[i];
    ruleDemand.source = rule.source;
    ruleDemand.ruleId = rule.id;
    ruleDemand.signalValid = signal.valid;

    if (!signal.valid && !applyLegacyFallback(rule.source, value)) {
      state.active[i] = false;
      continue;
    }

    const int sourcePercent = evaluateCurve(
      rule,
      value,
      target,
      deviceConfig.minimumPercent,
      deviceConfig.maximumPercent,
      state.active[i]
    );
    ruleDemand.percent = sourcePercent;
    ruleDemand.active = state.active[i];
    if (sourcePercent > demand.percent) {
      demand.percent = sourcePercent;
      demand.winningRuleIndex = i;
    }
    if (rule.source == SIGNAL_CLIMATE_TEMPERATURE_C) {
      demand.temperaturePercent = max(demand.temperaturePercent, sourcePercent);
    } else if (rule.source == SIGNAL_CLIMATE_HUMIDITY_PCT) {
      demand.humidityPercent = max(demand.humidityPercent, sourcePercent);
    }
  }

  if (demand.temperaturePercent > 0 && demand.humidityPercent > 0) demand.reason = "TEMP+HUM";
  else if (demand.temperaturePercent > 0) demand.reason = "TEMP";
  else if (demand.humidityPercent > 0) demand.reason = "HUM";
  else if (demand.winningRuleIndex >= 0) {
    demand.reason = demand.rules[demand.winningRuleIndex].ruleId;
  } else if (automationConfig.basePercent > 0) demand.reason = "BASE";
  else demand.reason = "IDLE";

  return demand;
}

static void appendRuleConfig(String& json, const FanSourceRuleConfig& rule) {
  json += "{";
  json += "\"id\":\"";
  json += rule.id;
  json += "\",\"signal\":\"";
  json += signalRegistry_name(rule.source);
  json += "\",\"enabled\":";
  json += rule.enabled ? "true" : "false";
  json += ",\"direction\":\"";
  json += fanControl_directionName(rule.direction);
  json += "\",\"target_source\":\"";
  json += fanControl_targetSourceName(rule.targetSource);
  json += "\",\"fixed_target\":";
  json += String(rule.fixedTarget, 1);
  json += ",\"lead_before_target\":";
  json += String(rule.leadBeforeTarget, 1);
  json += ",\"full_load_beyond_target\":";
  json += String(rule.fullLoadBeyondTarget, 1);
  json += ",\"hysteresis\":";
  json += String(rule.hysteresis, 1);
  json += ",\"curve\":\"";
  json += fanControl_curveName(rule.curve);
  json += "\"}";
}

static void appendLegacySourceConfig(
  String& json,
  const char* source,
  const FanSourceRuleConfig& rule
) {
  json += "\"";
  json += source;
  json += "\":{";
  json += "\"signal\":\"";
  json += signalRegistry_name(rule.source);
  json += "\",\"enabled\":";
  json += rule.enabled ? "true" : "false";
  json += ",\"lead_before_target\":";
  json += String(rule.leadBeforeTarget, 1);
  json += ",\"full_load_above_target\":";
  json += String(rule.fullLoadBeyondTarget, 1);
  json += ",\"hysteresis\":";
  json += String(rule.hysteresis, 1);
  json += ",\"curve\":\"";
  json += fanControl_curveName(rule.curve);
  json += "\"}";
}

static void appendFanConfig(
  String& json,
  int fan,
  const FanDeviceConfig& deviceConfig,
  const FanAutomationConfig& automationConfig
) {
  json += "\"fan";
  json += fan;
  json += "\":{";
  json += "\"device_profile\":\"STANDARD_PWM_TACHO\",";
  json += "\"default_mode\":\"";
  json += fanControl_modeName(automationConfig.defaultMode);
  json += "\",\"manual_percent\":";
  json += automationConfig.manualPercent;
  json += ",\"base_percent\":";
  json += automationConfig.basePercent;
  json += ",\"minimum_percent\":";
  json += deviceConfig.minimumPercent;
  json += ",\"maximum_percent\":";
  json += deviceConfig.maximumPercent;
  json += ",\"startup_boost_percent\":";
  json += deviceConfig.startupBoostPercent;
  json += ",\"startup_boost_ms\":";
  json += deviceConfig.startupBoostMs;
  json += ",\"minimum_run_ms\":";
  json += automationConfig.minimumRunMs;
  json += ",\"combine\":\"MAXIMUM\",";
  appendLegacySourceConfig(json, "temperature", automationConfig.rules[0]);
  json += ",";
  appendLegacySourceConfig(json, "humidity", automationConfig.rules[1]);
  json += ",\"rules\":[";
  const int ruleCount = constrain(
    automationConfig.ruleCount, 0, FAN_MAX_RULES_PER_CHANNEL);
  for (int i = 0; i < ruleCount; i++) {
    if (i > 0) json += ",";
    appendRuleConfig(json, automationConfig.rules[i]);
  }
  json += "]";
  json += "}";
}

void fanControl_appendJson(String& json) {
  json += "\"fan_control\":{";
  json += "\"curve_points\":";
  json += AUTO_CURVE_POINT_COUNT;
  json += ",\"sensor_failure_percent\":";
  json += FAN_SENSOR_FAIL_PERCENT;
  json += ",";
  for (int fan = 1; fan <= FAN_CHANNEL_COUNT; fan++) {
    if (fan > 1) json += ",";
    appendFanConfig(
      json,
      fan,
      STANDARD_FAN_DEVICE_CONFIG,
      fanControl_getAutomationConfig(fan)
    );
  }
  json += "}";
}
