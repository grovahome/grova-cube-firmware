#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <WiFi.h>

#include <cassert>
#include <iostream>

#include "modules/rest_mode.h"
#include "modules/wifi_ota.h"
char testLegacySsid[33] = "", testLegacyPassword[65] = "";
static unsigned long now = 0;
unsigned long millis() { return now; }
int main() {
  ESP.mac = 0x112233AABBCCULL;
  wifiOTA_begin();
  restMode_begin();
  assert(!wifiOTA_otaReady());
  assert(strlen(wifiOTA_setupPassword()) == 8);
  std::string first = wifiOTA_setupSSID();
  assert(first == "GROVA-CCBBAA332211");
  ESP.mac = 0x445566AABBCCULL;
  wifiOTA_begin();
  std::string second = wifiOTA_setupSSID();
  assert(second == "GROVA-CCBBAA665544" && second != first);
  assert(wifiOTA_submit("home", "password"));
  wifiOTA_loop();
  WiFi.link = WL_CONNECTED;
  wifiOTA_loop();
  now += 2100;
  wifiOTA_loop();
  assert(!wifiOTA_requiresSetup());
  assert(ArduinoOTA.hostname == second);
  assert(wifiOTA_otaReady());
  assert(wifiOTA_setupCloseDelayMs() == 5000);
  assert(wifiOTA_setupActive());
  now += 4999; wifiOTA_loop(); assert(wifiOTA_setupActive());
  now += 1; wifiOTA_loop(); assert(!wifiOTA_setupActive());
  now += 3000;
  assert(wifiOTA_requestSetup());
  assert(wifiOTA_setupCloseDelayMs() == 60000);
  assert(wifiOTA_isConnected());
  std::cout << "Founder WLAN and OTA names stay unique for equal MAC manufacturer bytes\n";
}
