#include "founder/serial.h"

#include <Arduino.h>

#include "founder/app.h"
#include "founder/display.h"
#include "modules/wifi_ota.h"
namespace founderSerial {
static char line[80];
static size_t length = 0;
static bool overflow = false, wasOpen = false;
static void handle() {
  line[length] = 0;
  if (!strcmp(line, "status")) {
    Serial.println(founderApp::status());
  } else if (!strcmp(line, "setup")) {
    founderApp::setRest(true);
    if (wifiOTA_requestSetup()) {
      // Credentials are printed only after an explicit physical USB request.
      Serial.printf("SSID: %s\nSetup key: %s\nIP: %s\n", wifiOTA_setupSSID(),
                    wifiOTA_setupPassword(), wifiOTA_setupIP());
    } else {
      Serial.println("Setup AP could not be started; retry in 10 seconds");
    }
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
    Serial.println("Commands: status | setup | display <rotation> <offset>");
  }
}
void loop() {
  bool open = bool(Serial);
  if (open && !wasOpen) {
    Serial.println("GROVA Founder USB: status | setup | display <rotation 0..3> <offset 0..95>");
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
}  // namespace founderSerial
