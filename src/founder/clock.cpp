#include <Arduino.h>
#include <Preferences.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "founder/rtc.h"
#include "modules/time_sync.h"
static Preferences settings;
static bool enabled = true, settingsOk = false, valid = false, readOk = false, writeOk = false,
            bootRtc = false;
static volatile bool ntpReceived = false;
static bool ntpKnown = false;
static uint32_t lastProbe = 0, lastWrite = 0;
static void synced(timeval*) {
  ntpReceived = true;
}
static bool restoreRtc() {
  uint32_t stamp = 0;
  readOk = enabled && founderRtc::read(stamp);
  valid = readOk;
  if (!readOk) {
    return false;
  }
  timeval tv = {};
  tv.tv_sec = stamp;
  if (settimeofday(&tv, nullptr) != 0) {
    return false;
  }
  bootRtc = true;
  return true;
}
void time_begin() {
  settingsOk = settings.begin("founder-time", false);
  if (settingsOk) {
    enabled = settings.getBool("rtc", true);
  }
  founderRtc::begin();
  setenv("TZ", TIMEZONE_TZ, 1);
  tzset();
  if (enabled) {
    restoreRtc();
  }
  sntp_set_time_sync_notification_cb(synced);
  configTzTime(TIMEZONE_TZ, "pool.ntp.org");
}
void time_loop() {
  bool networkSync = ntpReceived;
  if (networkSync) {
    ntpReceived = false;
    ntpKnown = true;
  }
  if (millis() - lastProbe >= 30000) {
    lastProbe = millis();
    if (!founderRtc::backupConfigured()) {
      founderRtc::begin();
    }
    uint32_t stamp = 0;
    readOk = enabled && founderRtc::read(stamp);
    valid = readOk;
    if (!ntpKnown && !bootRtc && readOk) {
      restoreRtc();
    }
  }
  if (enabled && ntpKnown &&
      (networkSync || millis() - lastWrite >= 21600000UL ||
       (!writeOk && millis() - lastWrite >= 30000))) {
    lastWrite = millis();
    writeOk = founderRtc::write(uint32_t(time(nullptr)));
    valid = writeOk;
  }
}
static tm local() {
  time_t now = time(nullptr);
  tm t = {};
  localtime_r(&now, &t);
  return t;
}
bool isTimeSynced() {
  return (bootRtc || ntpKnown) && time(nullptr) >= 1704067200;
}
int getHour() {
  return isTimeSynced() ? local().tm_hour : 0;
}
int getMinute() {
  return isTimeSynced() ? local().tm_min : 0;
}
int getYearDay() {
  return isTimeSynced() ? local().tm_yday : -1;
}
int getDateKey() {
  tm t = local();
  return isTimeSynced() ? (t.tm_year + 1900) * 1000 + t.tm_yday : -1;
}
unsigned long getEpochSeconds() {
  return isTimeSynced() ? time(nullptr) : 0;
}
unsigned long long getEpochMillis() {
  return uint64_t(getEpochSeconds()) * 1000;
}
bool time_settingsReady() {
  return settingsOk;
}
bool rtc_isEnabled() {
  return enabled;
}
bool rtc_isPresent() {
  return founderRtc::present();
}
bool rtc_hasValidTime() {
  return valid;
}
bool rtc_wasUsedForBoot() {
  return bootRtc;
}
bool rtc_lastReadOk() {
  return readOk;
}
bool rtc_lastWriteOk() {
  return writeOk;
}
const char* time_getSourceName() {
  return ntpKnown ? "ntp" : bootRtc ? "rtc" : "none";
}
bool rtc_setEnabled(bool value) {
  if (!settingsOk || settings.putBool("rtc", value) != 1) {
    return false;
  }
  enabled = value;
  if (enabled && !ntpKnown && !bootRtc) {
    restoreRtc();
  }
  if (!enabled) {
    readOk = false;
    valid = false;
  }
  return true;
}
