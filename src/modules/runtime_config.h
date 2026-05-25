#pragma once

#include <Arduino.h>

constexpr int FAN_CURVE_POINT_COUNT = 5;

struct FanCurvePoint {
  float tempOverC;
  int humOverPct;
  int fanPct;
};

void runtimeConfig_begin();
bool runtimeConfig_settingsReady();

float runtimeConfig_getTempMinC();
float runtimeConfig_getTempMaxC();
int runtimeConfig_getHumMinPct();
int runtimeConfig_getHumMaxPct();
FanCurvePoint runtimeConfig_getFanCurvePoint(int index);
int runtimeConfig_evaluateFanPercent(float tempOverC, float humOverPct);

void runtimeConfig_appendJson(String& json);
bool runtimeConfig_applyJson(const String& body, String& responseJson);
