#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <string.h>
#include <sys/time.h>
#include "time.h"
#include "config.h"

static const char* ntpServer = "pool.ntp.org";

static Preferences timeSettings;
static bool settingsReady = false;
static int lastHour = 0;
static int lastMinute = 0;
static int lastYearDay = -1;
static int lastYear = 0;
static bool timeSynced = false;
static bool rtcEnabled = GROVA_RTC_DEFAULT_ENABLED;
static bool rtcPresent = false;
static bool rtcValid = false;
static bool rtcBootSource = false;
static bool rtcReadOk = false;
static bool rtcWriteOk = false;
static unsigned long lastTimeLog = 0;
static unsigned long lastRtcProbe = 0;
static unsigned long lastRtcWrite = 0;
static const char* timeSource = "none";

static uint8_t bcdToDec(uint8_t value) {
  return ((value >> 4) * 10) + (value & 0x0F);
}

static uint8_t decToBcd(uint8_t value) {
  return ((value / 10) << 4) | (value % 10);
}

static bool rtcProbe() {
  Wire.beginTransmission(RTC_I2C_ADDR);
  rtcPresent = Wire.endTransmission() == 0;
  return rtcPresent;
}

static bool rtcReadTm(struct tm& out) {
  rtcReadOk = false;
  if (!rtcEnabled) {
    rtcPresent = false;
    rtcValid = false;
    return false;
  }
  if (!rtcProbe()) {
    rtcValid = false;
    return false;
  }

  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    rtcValid = false;
    return false;
  }

  if (Wire.requestFrom(RTC_I2C_ADDR, 7) != 7) {
    rtcValid = false;
    return false;
  }

  uint8_t seconds = Wire.read();
  uint8_t minutes = Wire.read();
  uint8_t hours = Wire.read();
  Wire.read(); // day of week
  uint8_t day = Wire.read();
  uint8_t month = Wire.read();
  uint8_t year = Wire.read();

  bool oscillatorStopped = (seconds & 0x80) != 0;
  int decodedYear = 2000 + bcdToDec(year);
  int decodedMonth = bcdToDec(month & 0x1F);
  int decodedDay = bcdToDec(day & 0x3F);
  int decodedHour = bcdToDec(hours & 0x3F);
  int decodedMinute = bcdToDec(minutes & 0x7F);
  int decodedSecond = bcdToDec(seconds & 0x7F);

  rtcValid =
    !oscillatorStopped &&
    decodedYear >= 2024 && decodedYear <= 2099 &&
    decodedMonth >= 1 && decodedMonth <= 12 &&
    decodedDay >= 1 && decodedDay <= 31 &&
    decodedHour >= 0 && decodedHour <= 23 &&
    decodedMinute >= 0 && decodedMinute <= 59 &&
    decodedSecond >= 0 && decodedSecond <= 59;

  if (!rtcValid) return false;

  memset(&out, 0, sizeof(out));
  out.tm_year = decodedYear - 1900;
  out.tm_mon = decodedMonth - 1;
  out.tm_mday = decodedDay;
  out.tm_hour = decodedHour;
  out.tm_min = decodedMinute;
  out.tm_sec = decodedSecond;
  out.tm_isdst = -1;
  rtcReadOk = true;
  return true;
}

static bool rtcWriteTm(const struct tm& timeinfo) {
  rtcWriteOk = false;
  if (!rtcEnabled || !rtcProbe()) return false;

  int year = timeinfo.tm_year + 1900;
  if (year < 2024 || year > 2099) return false;

  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(0x00);
  Wire.write(decToBcd(static_cast<uint8_t>(timeinfo.tm_sec)));
  Wire.write(decToBcd(static_cast<uint8_t>(timeinfo.tm_min)));
  Wire.write(decToBcd(static_cast<uint8_t>(timeinfo.tm_hour)));
  Wire.write(decToBcd(static_cast<uint8_t>(timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday)));
  Wire.write(decToBcd(static_cast<uint8_t>(timeinfo.tm_mday)));
  Wire.write(decToBcd(static_cast<uint8_t>(timeinfo.tm_mon + 1)));
  Wire.write(decToBcd(static_cast<uint8_t>(year - 2000)));
  rtcWriteOk = Wire.endTransmission() == 0;
  return rtcWriteOk;
}

static void applyTimeInfo(const struct tm& timeinfo, const char* source) {
  lastHour = timeinfo.tm_hour;
  lastMinute = timeinfo.tm_min;
  lastYearDay = timeinfo.tm_yday;
  lastYear = timeinfo.tm_year + 1900;
  timeSynced = true;
  timeSource = source;
}

static bool setSystemTimeFromRtc() {
  struct tm rtcTime;
  if (!rtcReadTm(rtcTime)) return false;

  time_t epoch = mktime(&rtcTime);
  if (epoch <= 1704067200) return false;

  timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  struct tm localTime;
  if (getLocalTime(&localTime, 50)) {
    applyTimeInfo(localTime, "rtc");
  } else {
    applyTimeInfo(rtcTime, "rtc");
  }
  rtcBootSource = true;
  Serial.println("RTC Zeit uebernommen");
  return true;
}

void time_begin() {
  settingsReady = timeSettings.begin("time", false);
  if (settingsReady) {
    rtcEnabled = timeSettings.getBool("rtc_enabled", GROVA_RTC_DEFAULT_ENABLED);
  } else {
    Serial.println("Time settings unavailable");
  }

  configTzTime(TIMEZONE_TZ, ntpServer);
  Serial.println("NTP Synchronisierung gestartet");

  if (rtcEnabled) {
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!setSystemTimeFromRtc()) {
      Serial.println(rtcPresent ? "RTC Zeit ungueltig" : "RTC nicht gefunden");
    }
  }
}

void time_loop() {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 10)) {
    bool firstSync = !timeSynced;
    const char* source = (strcmp(timeSource, "rtc") == 0 && WiFi.status() != WL_CONNECTED) ? "rtc" : "system";
    applyTimeInfo(timeinfo, source);

    if (firstSync) {
      Serial.println("Zeit synchronisiert");
    }

    if (
      rtcEnabled &&
      rtcPresent &&
      WiFi.status() == WL_CONNECTED &&
      millis() > 60000UL &&
      (!rtcWriteOk || !rtcValid || millis() - lastRtcWrite >= RTC_WRITE_INTERVAL_MS)
    ) {
      lastRtcWrite = millis();
      rtcWriteTm(timeinfo);
    }
    return;
  }

  if (rtcEnabled && millis() - lastRtcProbe >= RTC_PROBE_INTERVAL_MS) {
    lastRtcProbe = millis();
    setSystemTimeFromRtc();
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
bool time_settingsReady() { return settingsReady; }
bool rtc_isEnabled() { return rtcEnabled; }
bool rtc_isPresent() { return rtcPresent; }
bool rtc_hasValidTime() { return rtcValid; }
bool rtc_wasUsedForBoot() { return rtcBootSource; }
bool rtc_lastReadOk() { return rtcReadOk; }
bool rtc_lastWriteOk() { return rtcWriteOk; }
const char* time_getSourceName() { return timeSource; }

bool rtc_setEnabled(bool enabled) {
  rtcEnabled = enabled;
  if (settingsReady) {
    timeSettings.putBool("rtc_enabled", rtcEnabled);
  }

  if (!rtcEnabled) {
    rtcPresent = false;
    rtcValid = false;
    rtcReadOk = false;
    timeSource = timeSynced ? "system" : "none";
    return true;
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  lastRtcProbe = 0;
  return setSystemTimeFromRtc() || rtcPresent;
}
