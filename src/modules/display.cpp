#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "firmware_info.h"
#include "modules/alarms.h"
#include "modules/climate.h"
#include "modules/fan.h"
#include "modules/grow_mode.h"
#include "modules/light.h"
#include "modules/local_run.h"
#include "modules/pump_scheduler.h"
#include "modules/rest_mode.h"
#include "modules/runtime_config.h"
#include "modules/sensors.h"
#include "modules/stability.h"
#include "modules/time_sync.h"
#include "modules/ui.h"
#include "modules/wifi_ota.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static bool displayReady = false;
static bool displaySleeping = false;
static unsigned long lastUpdate = 0;
static unsigned long lastActivity = 0;
static const unsigned long DISPLAY_UPDATE_MS = 250;

static void drawHeader(const char* title, float temp, float hum) {
  display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 1);
  display.print(title);

  display.setCursor(94, 1);
  display.print(ui_getScreen() + 1);
  display.print("/");
  display.print(UI_SCREEN_COUNT);

  if (ui_isEditing()) {
    display.setCursor(66, 1);
    display.print("EDIT");
  } else if (restMode_isEnabled()) {
    display.setCursor(66, 1);
    display.print("REST");
  } else if (alarms_hasWarning(temp, hum)) {
    display.setCursor(62, 1);
    display.print("WARN");
  }

  display.setTextColor(SSD1306_WHITE);
}

static void drawLine(int line, const char* label) {
  display.setCursor(0, 13 + (line * 10));
  display.print(label);
}

static void drawStatusPage(float temp, float hum, int fanPercent, bool lightOn) {
  drawLine(0, "T ");
  display.print(temp, 1);
  display.print("/");
  display.print(getTargetTemp(), 1);
  display.print("C");

  drawLine(1, "H ");
  display.print(hum, 0);
  display.print("/");
  display.print(getTargetHum(), 0);
  display.print("%");

  drawLine(2, "Grow ");
  display.print(growMode_getName());
  if (restMode_isEnabled()) display.print(" REST");

  drawLine(3, "Fan ");
  display.print(fanPercent);
  display.print("% ");
  display.print(ui_getFanModeName());

  drawLine(4, "Light ");
  display.print(lightOn ? "ON " : "OFF ");
  display.print(restMode_isEnabled() ? "REST" : ui_getLightModeName());
}

static void drawGrowPage() {
  drawLine(0, "Mode ");
  display.print(growMode_getName());
  if (restMode_isEnabled()) display.print(" + REST");

  drawLine(1, "Effect ");
  display.print(growMode_getEffectName());

  drawLine(2, "Light ");
  display.print(light_getReasonName());

  drawLine(3, "Pump ");
  display.print(pumpScheduler_getReasonName());

  drawLine(4, ui_isEditing() ? "Turn mode Click ok" : "Click next Hold edit");
}

static void drawFanPage(int fanPercent) {
  drawLine(0, "Mode ");
  display.print(ui_getFanModeName());

  drawLine(1, "Now ");
  display.print(fanPercent);
  display.print("%  Target ");
  display.print(getFanTargetPercent());
  display.print("%");

  drawLine(2, "RPM ");
  display.print(getFanRPM());
#if FAN2_ENABLED
  display.print("/");
  display.print(getFan2RPM());
#endif
  display.print(" Tach ");
  display.print(fan_getTachoStatusName());

  drawLine(3, "Reason ");
  display.print(fan_getReasonName());

  if (restMode_isEnabled()) {
    drawLine(4, "Rest mode off");
  } else if (sensors_hasFault()) {
    drawLine(4, "Sensor fault safe");
  } else if (fan_hasTachoFault()) {
    drawLine(4, "Fan RPM warning");
  } else {
    drawLine(4, ui_isEditing() ? "Turn speed Click ok" : "Click edit Hold mode");
  }
}

static void drawClimateSettingLine(int line, int settingIndex) {
  bool selected = ui_getClimateSettingIndex() == settingIndex;

  drawLine(line, selected ? "> " : "  ");
  display.print(climate_getSettingName(settingIndex));
  display.print(" ");

  if (settingIndex == CLIMATE_DAY_TEMP || settingIndex == CLIMATE_NIGHT_TEMP) {
    display.print(climate_getSettingValue(settingIndex), 1);
  } else {
    display.print(climate_getSettingValue(settingIndex), 0);
  }

  display.print(climate_getSettingUnit(settingIndex));
}

static void drawClimatePage() {
  drawClimateSettingLine(0, CLIMATE_DAY_TEMP);
  drawClimateSettingLine(1, CLIMATE_NIGHT_TEMP);
  drawClimateSettingLine(2, CLIMATE_DAY_HUM);
  drawClimateSettingLine(3, CLIMATE_NIGHT_HUM);

  drawLine(4, ui_isEditing() ? "Turn value Click ok" : "Click select Hold edit");
}

static void drawLightPage(bool lightOn) {
  drawLine(0, ui_getLightSettingIndex() == LIGHT_SETTING_MODE ? "> State " : "  State ");
  display.print(lightOn ? "ON" : "OFF");

  drawLine(1, "  Mode ");
  display.print(ui_getLightModeName());

  drawLine(2, ui_getLightSettingIndex() == LIGHT_SETTING_ON_HOUR ? "> On " : "  On ");
  if (light_getOnHour() < 10) display.print("0");
  display.print(light_getOnHour());
  display.print(":00");

  drawLine(3, ui_getLightSettingIndex() == LIGHT_SETTING_OFF_HOUR ? "> Off " : "  Off ");
  if (light_getOffHour() < 10) display.print("0");
  display.print(light_getOffHour());
  display.print(":00");

  drawLine(4, ui_isEditing() ? "Turn value Click ok" : "Click select Hold act");
}

static void drawPumpPage() {
  drawLine(0, ui_getPumpSettingIndex() == PUMP_SETTING_TEST ? "> Mode " : "  Mode ");
  display.print(pumpScheduler_getModeName());
  if (pumpScheduler_isRunning()) {
    display.print(" ");
    display.print(pumpScheduler_getRemainingSeconds());
    display.print("s");
  }

  drawLine(1, ui_getPumpSettingIndex() == PUMP_SETTING_HOUR ? "> Hour " : "  Hour ");
  if (pumpScheduler_getRunHour() < 10) display.print("0");
  display.print(pumpScheduler_getRunHour());
  display.print(":");
  if (pumpScheduler_getRunMinute() < 10) display.print("0");
  display.print(pumpScheduler_getRunMinute());

  drawLine(2, ui_getPumpSettingIndex() == PUMP_SETTING_MINUTE ? "> Minute " : "  Minute ");
  if (pumpScheduler_getRunMinute() < 10) display.print("0");
  display.print(pumpScheduler_getRunMinute());
  display.print("  Dur ");
  display.print(pumpScheduler_getRunDurationSeconds());
  display.print("s");

  drawLine(3, ui_getPumpSettingIndex() == PUMP_SETTING_DURATION ? "> Dur " : "  Dur ");
  display.print(pumpScheduler_getRunDurationSeconds());
  display.print("s Runs ");
  display.print(pumpScheduler_getRunsToday());
  display.print("/");
  display.print(pumpScheduler_getMaxRunsPerDay());

  drawLine(4, "State ");
  if (restMode_isEnabled()) {
    display.print("REST OFF");
  } else if (!isTimeSynced()) {
    display.print("sync...");
  } else if (pumpScheduler_isStartupLocked()) {
    display.print("boot lock");
  } else {
    display.print(pumpScheduler_getReasonName());
  }
}

static void drawSensorsPage(float temp, float hum) {
  drawLine(0, "Temp ");
  display.print(temp, 1);
  display.print("C");

  drawLine(1, "Hum ");
  display.print(hum, 1);
  display.print("%");

  drawLine(2, "Status ");
  display.print(sensors_getStatusName());

  drawLine(3, "Src ");
  display.print(sensors_getSourceName());

  drawLine(4, "P ");
  if (sensors_hasPressure()) {
    display.print(sensors_getPressureHpa(), 0);
    display.print(" ");
    display.print(sensors_getPressureSourceName());
  } else {
    display.print("none");
  }
}

static void drawDiagPage(float temp, float hum, bool lightOn) {
  drawLine(0, "Warn ");
  display.print(alarms_getPrimaryWarning(temp, hum));

  drawLine(1, "Fan ");
  display.print(fan_getReasonName());
  display.print(" ");
  display.print(getFanTargetPercent());
  display.print("%");

  drawLine(2, "Light ");
  display.print(lightOn ? "ON " : "OFF ");
  display.print(light_getReasonName());

  drawLine(3, "Pump ");
  display.print(pumpScheduler_getReasonName());

  drawLine(4, "Mode ");
  display.print(growMode_getName());
  if (restMode_isEnabled()) display.print(" REST");
}

static void drawSystemPage() {
  drawLine(0, "WiFi ");
  display.print(wifiOTA_isConnected() ? "OK" : "FAIL");

  drawLine(1, "IP ");
  display.print(wifiOTA_getIP());

  drawLine(2, "FW ");
  display.print(GROVA_FIRMWARE_VERSION);

  drawLine(3, "Build ");
  display.print(GROVA_BUILD_TIME);

  drawLine(4, "Time ");
  display.print(isTimeSynced() ? "OK " : "-- ");
  display.print("RTC ");
  if (!rtc_isEnabled()) {
    display.print("OFF ");
  } else if (!rtc_isPresent()) {
    display.print("MISS ");
  } else {
    display.print(rtc_hasValidTime() ? "OK " : "BAD ");
  }
  display.print("C ");
  bool cfgOk =
    ui_settingsReady() &&
    growMode_settingsReady() &&
    climate_settingsReady() &&
    light_settingsReady() &&
    pumpScheduler_settingsReady() &&
    restMode_settingsReady() &&
    localRun_settingsReady() &&
    runtimeConfig_settingsReady() &&
    time_settingsReady();
  display.print(cfgOk ? "OK" : "FAIL");
}

void display_begin() {
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED nicht gefunden");
    return;
  }

  displayReady = true;
  displaySleeping = false;
  lastActivity = millis();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 20);
  display.println("vGrow Start...");
  display.display();

  delay(500);
}

bool display_wakeFromUser() {
  if (!displayReady) return false;

  bool wasSleeping = displaySleeping;
  lastActivity = millis();

  if (displaySleeping) {
    displaySleeping = false;
    display.ssd1306_command(SSD1306_DISPLAYON);
    lastUpdate = 0;
  }

  return wasSleeping;
}

void display_loop(float temp, float hum, int fanPercent, bool lightOn) {
  if (!displayReady) return;

  if (!displaySleeping && millis() - lastActivity >= DISPLAY_SLEEP_MS) {
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    displaySleeping = true;
    return;
  }

  if (displaySleeping) return;

  if (millis() - lastUpdate < DISPLAY_UPDATE_MS) return;
  lastUpdate = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  drawHeader(ui_getScreenName(), temp, hum);

  switch (ui_getScreen()) {
    case UI_SCREEN_STATUS:
      drawStatusPage(temp, hum, fanPercent, lightOn);
      break;
    case UI_SCREEN_GROW:
      drawGrowPage();
      break;
    case UI_SCREEN_FAN:
      drawFanPage(fanPercent);
      break;
    case UI_SCREEN_CLIMATE:
      drawClimatePage();
      break;
    case UI_SCREEN_LIGHT:
      drawLightPage(lightOn);
      break;
    case UI_SCREEN_PUMP:
      drawPumpPage();
      break;
    case UI_SCREEN_SENSORS:
      drawSensorsPage(temp, hum);
      break;
    case UI_SCREEN_DIAG:
      drawDiagPage(temp, hum, lightOn);
      break;
    case UI_SCREEN_SYSTEM:
      drawSystemPage();
      break;
    default:
      drawStatusPage(temp, hum, fanPercent, lightOn);
      break;
  }

  display.display();
}
