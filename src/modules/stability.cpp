#include <Arduino.h>
#include <math.h>
#include "modules/alarms.h"
#include "modules/fan.h"
#include "modules/sensors.h"

// ===== HEALTH STATE =====
static bool systemHealthy = true;

static unsigned long lastCheck = 0;

// ===== LIMITS =====
const float TEMP_MIN = 0;
const float TEMP_MAX = 45;

const float HUM_MIN = 0;
const float HUM_MAX = 100;

void stability_begin() {
  systemHealthy = true;
  Serial.println("Stability Layer gestartet");
}

void stability_loop() {
  // Extend here later with WiFi, fan, and other health checks.

  if (millis() - lastCheck < 5000) return;
  lastCheck = millis();

  systemHealthy = !alarms_hasWarning(getTemp(), getHum());

  Serial.print("System Health ");
  Serial.println(systemHealthy ? "OK" : "WARN");
}

bool isSystemHealthy() {
  return systemHealthy;
}

// ===== SENSOR SANITIZER =====
float safeTemp(float t) {

  if (isnan(t)) return 22.0;
  if (t < TEMP_MIN) return 22.0;
  if (t > TEMP_MAX) return 30.0;

  return t;
}

float safeHum(float h) {

  if (isnan(h)) return 60.0;
  if (h < HUM_MIN) return 60.0;
  if (h > HUM_MAX) return 80.0;

  return h;
}
