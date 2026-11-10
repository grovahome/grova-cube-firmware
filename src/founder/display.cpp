#include "founder/display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "boards/grova_founder_s3.h"
#include "founder/app.h"
#include "founder/i2c_bus.h"
#include "modules/fan.h"
#include "modules/rest_mode.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
#include "modules/wifi_ota.h"
namespace founderDisplay {
static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool ready = false;
static int page = 0;
static uint32_t drawAt = 0, probeAt = 0;
void begin() {
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
      if (address) {
        ready = oled.begin(SSD1306_SWITCHCAPVCC, address, true, false);
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
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
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
  oled.display();
}
}  // namespace founderDisplay
