#include "founder/led.h"
#include <Arduino.h>
#include <cstring>
#include "boards/grova_founder_s3.h"
#include "founder/led_policy.h"
#include "founder/app.h"
#include "founder/outputs.h"
#include "modules/rest_mode.h"
#include "modules/wifi_ota.h"
namespace founderLed {
static Policy policy;
static Color previous = {255, 255, 255};
static uint32_t frameAt = 0;
void begin() {
  previous = {255, 255, 255};
  frameAt = millis() - 100;
  neopixelWrite(founder::RGB_DATA, 0, 0, 0);
}
bool startTest() {
  if (!restMode_isEnabled()) return false;
  policy.startTest(millis());
  frameAt = millis() - 100;
  return true;
}
void loop() {
  const uint32_t now = millis();
  if (uint32_t(now - frameAt) < 100) return;
  frameAt = now;
  const bool connected = wifiOTA_isConnected();
  const bool error = founderApp::hasFault() ||
      (!connected && strlen(wifiOTA_setupError()) != 0);
  const Color next = policy.color(now, {error, wifiOTA_setupActive(), wifiOTA_isTesting(), connected});
  if (!same(previous, next)) {
    neopixelWrite(founder::RGB_DATA, next.r, next.g, next.b);
    previous = next;
  }
}
}  // namespace founderLed
