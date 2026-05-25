#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "config.h"

static const char* ntpServer = "pool.ntp.org";

static int lastHour = 0;
static int lastMinute = 0;
static int lastYearDay = -1;
static int lastYear = 0;
static bool timeSynced = false;
static unsigned long lastTimeLog = 0;

void time_begin() {
  configTzTime(TIMEZONE_TZ, ntpServer);
  Serial.println("NTP Synchronisierung gestartet");
}

void time_loop() {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 10)) {
    lastHour = timeinfo.tm_hour;
    lastMinute = timeinfo.tm_min;
    lastYearDay = timeinfo.tm_yday;
    lastYear = timeinfo.tm_year + 1900;

    if (!timeSynced) {
      timeSynced = true;
      Serial.println("Zeit synchronisiert");
    }
    return;
  }

  if (!timeSynced && millis() - lastTimeLog >= 5000) {
    lastTimeLog = millis();
    Serial.println("Warte auf NTP Zeit...");
  }
}

int getHour() { return lastHour; }
int getMinute() { return lastMinute; }
int getYearDay() { return lastYearDay; }
int getDateKey() {
  if (!timeSynced || lastYear <= 0 || lastYearDay < 0) return -1;
  return (lastYear * 1000) + lastYearDay;
}
bool isTimeSynced() { return timeSynced; }
