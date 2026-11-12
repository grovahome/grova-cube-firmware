#include "founder/serial.h"

#include <Arduino.h>
#include <WiFi.h>

#include "founder/app.h"
#include "founder/display.h"
#include "founder/i2c_bus.h"
#include "founder/led.h"
#include "modules/rest_mode.h"
#include "modules/wifi_ota.h"
namespace founderSerial {
static char line[80];
static size_t length = 0;
static bool overflow = false, wasOpen = false;
static void scanI2c() {
  if (!restMode_isEnabled()) {
    Serial.println("I2C scan requires Rest mode");
    return;
  }
  bool root[128] = {};
  for (int channel = -1; channel < 8; ++channel) {
    if (channel >= 0 && !founderBus::hasMux())
      break;
    founderBus::Scope bus(channel);
    if (!bus) {
      Serial.printf("I2C bus %d unavailable\n", channel);
      continue;
    }
    Serial.printf("I2C %s %d:\n", channel < 0 ? "root" : "mux", channel);
    for (unsigned address = 8; address < 120; ++address) {
      const bool ack = founderBus::probe(address);
      if (channel < 0)
        root[address] = ack;
      if (ack && (channel < 0 || !root[address]))
        Serial.printf("  ACK 0x%02X\n", address);
    }
  }
  Serial.println(
      "Scan complete; mux isolated. ACK does not identify a device.");
}
static void handle() {
  line[length] = 0;
  if (!strcmp(line, "status")) {
    Serial.println(founderApp::status());
  } else if (!strcmp(line, "led test")) {
    Serial.println(founderLed::startTest() ? "LED test: red 3s, green 3s, blue 3s, white 1.5s, off 1.5s, then status" : "LED test requires Rest mode");
  } else if (!strcmp(line, "wifi")) {
    Serial.printf("WiFi state=%s error=%s connected=%d testing=%d storage=%d AP=%d IP=%s radio=%d reason=%u begin=%d\n",
      wifiOTA_setupState(), wifiOTA_setupError(), wifiOTA_isConnected(), wifiOTA_isTesting(),
      wifiOTA_storageReady(), wifiOTA_setupActive(), wifiOTA_getIP(), int(WiFi.status()), wifiOTA_disconnectReason(), wifiOTA_beginResult());
  } else if (!strcmp(line, "i2c")) {
    scanI2c();
  } else if (!strcmp(line, "setup")) {
    founderApp::setRest(true);
    if (wifiOTA_requestSetup()) {
      // Credentials are printed only after an explicit physical USB request.
      Serial.printf("SSID: %s\nSetup key: %s\nIP: %s\n", wifiOTA_setupSSID(),
                    wifiOTA_setupPassword(), wifiOTA_setupIP());
    } else {
      Serial.println("Setup AP could not be started; retry in 10 seconds");
    }
  } else if (!strncmp(line, "panel ", 6)) {
    if (!restMode_isEnabled()) Serial.println("Panel selection requires Rest mode");
    else if (founderDisplay::selectController(line + 6)) Serial.println("Panel profile saved");
    else Serial.println("Use panel ssd1306 | panel ssd1317");
  } else if (!strncmp(line, "display ", 8)) {
    unsigned rotation = 0, offset = 0;
    char extra = 0;
    if (sscanf(line, "display %u %u %c", &rotation, &offset, &extra) == 2 &&
        founderDisplay::configure(rotation, offset)) {
      Serial.println("Display configuration saved");
    } else {
      Serial.println("Use display <rotation 0..3> <offset 0..95>");
    }
  } else if (length) {
    Serial.println(
        "Commands: status | i2c | setup | display <rotation> <offset>");
  }
}
void loop() {
  bool open = bool(Serial);
  if (open && !wasOpen) {
    Serial.println("GROVA Founder USB: status | i2c | setup | display "
                   "<rotation 0..3> <offset 0..95>");
  }
  wasOpen = open;
  // Bound serial work so a busy USB host cannot starve sensor/control updates.
  for (unsigned i = 0; i < 80 && Serial.available(); ++i) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (overflow) {
        Serial.println("Command too long");
      } else if (length) {
        handle();
      }
      length = 0;
      overflow = false;
    } else if (length < sizeof(line) - 1) {
      line[length++] = c;
    } else {
      overflow = true;
    }
  }
}
} // namespace founderSerial
