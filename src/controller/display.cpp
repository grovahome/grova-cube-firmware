#include "controller/display.h"

#include <Preferences.h>
#include <Wire.h>
#include <cstring>

#include "boards/grova_core_s3.h"
#include "drivers/ssd1317.h"
#include "controller/app.h"
#include "controller/boot_animation.h"
#include "controller/i2c_bus.h"
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
static bool useSSD1306 = false;
const char* controller() { return useSSD1306 ? "SSD1306" : "SSD1317"; }
static bool ready = false;
static uint8_t activeAddress = 0;
static founderBoot::Sequence welcome;
static bool lastFrameWelcome = false;
bool present() {
  return ready;
}
static int page = 0;
static uint32_t drawAt = 0, probeAt = 0;
static bool apply(unsigned rotation, unsigned offset, bool ssd1306) {
  if (rotation > 3 || offset > (ssd1306 ? 63U : 95U) || !storageReady) {
    return false;
  }
  uint16_t packed = (ssd1306 ? 0x8000 : 0) | (rotation << 8) | offset;
  if (displaySettings.putBytes("panel", &packed, sizeof(packed)) != sizeof(packed)) {
    return false;
  }
  welcome.dismiss();
  useSSD1306 = ssd1306;
  rotate = rotation;
  rowOffset = offset;
  oled.setRotation(rotate);
  ready = false;
  probeAt = millis() - 10000;
  return true;
}
bool configure(unsigned rotation, unsigned offset) { return apply(rotation, offset, useSSD1306); }
bool selectController(const char* name) {
  if (!strcmp(name, "ssd1306")) return apply(rotate, 0, true);
  if (!strcmp(name, "ssd1317")) return apply(rotate, 80, false);
  return false;
}
void begin() {
  ready = false;
  activeAddress = 0;
  welcome.reset(millis());
  lastFrameWelcome = false;
  storageReady = displaySettings.begin("founder-oled", false);
  uint16_t packed = 80;
  if (storageReady && displaySettings.getBytesLength("panel") == sizeof(packed)) {
    uint16_t stored = 0;
    if (displaySettings.getBytes("panel", &stored, sizeof(stored)) == sizeof(stored) &&
        (stored & 0x7C00) == 0 && (stored & 255) <= ((stored & 0x8000) ? 63 : 95)) {
      packed = stored;
    }
  }
  useSSD1306 = (packed & 0x8000) != 0;
  rotate = (packed >> 8) & 3;
  rowOffset = packed & 255;
  oled.setRotation(rotate);
  probeAt = millis() - 10000;
  drawAt = millis() - 1000;
}
void navigate(int delta) {
  if (delta) {
    welcome.dismiss();
  }
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
        ready = oled.begin(address, rowOffset, useSSD1306);
        if (ready) {
          welcome.found(millis());
        }
      }
    }
  }
  const bool showingWelcome = welcome.active(millis());
  const uint32_t interval = showingWelcome ? founderBoot::FrameMs : lastFrameWelcome ? 0 : 500;
  if (!ready || millis() - drawAt < interval) {
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
  if (showingWelcome) {
    founderBoot::draw(oled, welcome.elapsed(millis()));
    lastFrameWelcome = true;
    if (!oled.display()) {
      ready = false;
    }
    return;
  }
  lastFrameWelcome = false;
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
