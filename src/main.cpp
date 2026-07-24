#include <Arduino.h>

// =====================
// MODULES
// =====================
#include "modules/wifi_ota.h"
#include "modules/time_sync.h"
#include "modules/grow_mode.h"
#include "modules/runtime_config.h"
#include "modules/light.h"
#include "modules/sensors.h"
#include "modules/fan.h"
#include "modules/climate.h"
#include "modules/pump_scheduler.h"
#include "modules/outputs.h"
#include "modules/stability.h"
#include "modules/display.h"
#include "modules/encoder.h"
#include "modules/ui.h"
#include "modules/web_status.h"
#include "modules/mqtt_client.h"
// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  fan_preinit();
  delay(500);

  Serial.println("\n================================");
  Serial.println("vGrow SYSTEM START");
  Serial.println("================================");

  // Network + OTA
  wifiOTA_begin();

  // Time (NTP)
  time_begin();

  // Stability layer
  stability_begin();

  // System modules
  runtimeConfig_begin();
  outputs_begin();
  growMode_begin();
  light_begin();
  sensors_begin();
  fan_begin();
  climate_begin();
  ui_begin();
  display_begin();
  encoder_begin();
  webStatus_begin();
  mqttClient_begin();
  // Pump scheduler
  pumpScheduler_begin();

  Serial.println("System bereit.");
}

// =========================
// LOOP
// =========================
void loop() {

  // OTA + WiFi
  wifiOTA_loop();
  webStatus_loop();
  mqttClient_loop();

  // Time update
  time_loop();

  // Stability check
  stability_loop();

  // Target values based on light phase
  climate_loop();

  // Read sensors
  sensors_loop();

  // Time-based light control
  light_loop();

  // Fan control
  fan_loop(getTemp(), getHum());

  ui_loop();
  encoder_loop();

  // Display
  display_loop(
  getTemp(),
  getHum(),
  getFanPercent(),
  isLightOn()
  );

  // Watering once per day
  pumpScheduler_loop();

  // Small CPU relief
  delay(10);
}
