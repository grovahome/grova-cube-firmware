#include "founder/display.h"

#include <Preferences.h>
#include <Wire.h>

#include "boards/grova_founder_s3.h"
#include "drivers/ssd1317.h"
#include "founder/app.h"
#include "founder/i2c_bus.h"
#include "modules/fan.h"
#include "modules/rest_mode.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
#include "modules/wifi_ota.h"
namespace founderDisplay {
static GrovaSSD1317 oled;
static Preferences displaySettings;
static bool storageReady = false;
static unsigned rotate = 0, rowOffset = 80;
unsigned rotation() {
  return rotate;
}
unsigned offset() {
  return rowOffset;
}
static bool ready = false;
static uint8_t activeAddress = 0;
bool present() {
  return ready;
}
static int page = 0;
static uint32_t drawAt = 0, probeAt = 0;
bool configure(unsigned rotation, unsigned offset) {
  if (rotation > 3 || offset > 95 || !storageReady) {
    return false;
  }
  uint16_t packed = (rotation << 8) | offset;
  if (displaySettings.putBytes("panel", &packed, sizeof(packed)) != sizeof(packed)) {
    return false;
  }
  rotate = rotation;
  rowOffset = offset;
  oled.setRotation(rotate);
  ready = false;
  probeAt = millis() - 10000;
  return true;
}
void begin() {
  storageReady = displaySettings.begin("founder-oled", false);
  uint16_t packed = 80;
  if (storageReady && displaySettings.getBytesLength("panel") == sizeof(packed)) {
    uint16_t stored = 0;
    if (displaySettings.getBytes("panel", &stored, sizeof(stored)) == sizeof(stored) &&
        (stored >> 8) <= 3 && (stored & 255) <= 95) {
      packed = stored;
    }
  }
  rotate = packed >> 8;
  rowOffset = packed & 255;
  oled.setRotation(rotate);
  probeAt = millis() - 10000;
  drawAt = millis() - 1000;
}
void navigate(int delta) {
  page = (page + delta + 4) % 4;
}
void loop() {
  if (!ready && millis() - probeAt >= 10000) {
    probeAt = millis();
    founderBus::Scope bus;
    if (bus) {
      uint8_t address = founderBus::probe(0x3C) ? 0x3C : founderBus::probe(0x3D) ? 0x3D : 0;
      activeAddress = address;
      if (address) {
        ready = oled.begin(address, rowOffset);
      }
    }
  }
  if (!ready || millis() - drawAt < 500) {
    return;
  }
  drawAt = millis();
  founderBus::Scope bus;
  if (!bus) {
    return;
  }
  if (!founderBus::probe(activeAddress)) {
    ready = false;
    return;
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(0, 0);
  if (wifiOTA_setupActive()) {
    oled.println("GROVA Wi-Fi Setup");
    oled.println(wifiOTA_setupSSID());
    oled.println(wifiOTA_setupPassword());
    oled.println(wifiOTA_setupIP());
    oled.println(wifiOTA_setupState());
    if (wifiOTA_isConnected()) {
      oled.println(wifiOTA_getIP());
    }
  } else {
    oled.print("GROVA ");
    oled.println(restMode_isEnabled() ? "REST" : "ACTIVE");
    if (page == 0) {
      for (int i = 0; i < 3; ++i) {
        auto id = static_cast<SignalId>(i);
        auto v = signalRegistry_read(id);
        oled.print(i == 0 ? "Temp " : i == 1 ? "Hum  " : "Pres ");
        if (v.valid) {
          oled.print(v.value, 1);
        } else {
          oled.print("--");
        }
        oled.print(' ');
        oled.println(signalRegistry_unit(id));
      }
      oled.println(wifiOTA_getIP());
    } else if (page == 1) {
      oled.println("Generic outputs");
      oled.println("OUT12_1  OUT5_1");
      oled.println("OUT12_2");
      oled.println("0-10V OUT 1 / 2");
      oled.println("Control via webapp");
    } else if (page == 2) {
      oled.printf("Fan1 %d%% %d RPM\n", getFanPercent(), getFanRPM());
      oled.printf("Fan2 %d%% %d RPM\n", getFan2Percent(), getFan2RPM());
      oled.println(fan_getTachoStatusName());
    } else {
      oled.println("RV-3032 RTC");
      oled.println(rtc_hasValidTime() ? "Time valid" : "Time not valid");
      oled.println(time_getSourceName());
      oled.printf("%02d:%02d\n", getHour(), getMinute());
      oled.println(founderApp::deviceId());
    }
  }
  if (!oled.display()) {
    ready = false;
  }
}
}  // namespace founderDisplay
