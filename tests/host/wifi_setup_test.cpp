// Actual Wi-Fi coordinator and Rest storage; radio/clock/OTA/NVS adapters only are mocked.
#include <cassert>
#include <iostream>
#include "WiFi.h"
#include "ArduinoOTA.h"
#include "Preferences.h"
#include "modules/wifi_ota.h"
#include "modules/wifi_credentials.h"
#include "modules/rest_mode.h"
char testLegacySsid[33] = "", testLegacyPassword[65] = "";
static uint32_t clockMs = 0;
unsigned long millis() { return clockMs; }
void pump_inhibit() {}
static void tick(uint32_t ms = 1) { clockMs += ms; wifiOTA_loop(); }
static void boot() { wifiOTA_begin(); restMode_begin(); if (wifiOTA_requiresSetup()) restMode_setEnabled(true, true); }
static void connected() { WiFi.link = WL_CONNECTED; tick(); tick(2000); }
static void saveOriginal() {
  boot(); assert(wifiOTA_submit("original", "originalpass")); tick(); connected();
  assert(!wifiOTA_requiresSetup());
}
int main(int argc, char** argv) {
  assert(argc == 2); const std::string scenario = argv[1];
  if (scenario == "first_boot") {
    boot(); assert(wifiOTA_requiresSetup()); assert(wifiOTA_setupActive());
    assert(restMode_isEnabled()); assert(strlen(wifiOTA_setupPassword()) == 12);
    assert(ArduinoOTA.begins == 0); const std::string key = wifiOTA_setupPassword();
    boot(); assert(key == wifiOTA_setupPassword()); assert(WiFi.beginCalls == 0);
  } else if (scenario == "legacy_fallback") {
    strcpy(testLegacySsid, "old-network"); strcpy(testLegacyPassword, "old-password");
    boot(); assert(!wifiOTA_requiresSetup()); assert(!wifiOTA_setupActive());
    assert(WiFi.target == "old-network"); connected();
    assert(ArduinoOTA.begins == 1); assert(!restMode_isEnabled());
    assert(!testNvs::data.count("grova-wifi/credentials"));
  } else if (scenario == "saved_precedence") {
    saveOriginal(); strcpy(testLegacySsid, "obsolete"); strcpy(testLegacyPassword, "old-password");
    boot(); assert(WiFi.target == "original"); assert(!wifiOTA_requiresSetup());
  } else if (scenario == "success_and_ap_close") {
    saveOriginal(); assert(wifiOTA_setupActive()); assert(restMode_isEnabled());
    tick(59999); assert(wifiOTA_setupActive()); tick(1); assert(!wifiOTA_setupActive());
    boot(); assert(WiFi.target == "original"); assert(!wifiOTA_requiresSetup()); assert(restMode_isEnabled());
  } else if (scenario == "failed_trial_preserves_old" || scenario == "storage_failure") {
    saveOriginal(); const auto before = testNvs::data["grova-wifi/credentials"];
    assert(wifiOTA_submit("replacement", "new-password"));
    assert(!wifiOTA_submit("concurrent", "password")); tick();
    if (scenario == "storage_failure") { testNvs::failKey = "grova-wifi/credentials"; connected(); }
    else tick(30000);
    assert(!wifiOTA_isTesting()); assert(testNvs::data["grova-wifi/credentials"] == before);
    assert(WiFi.target == "original"); assert(strlen(wifiOTA_setupError()));
    testNvs::failKey.clear(); boot(); assert(WiFi.target == "original");
  } else if (scenario == "power_loss_during_trial") {
    saveOriginal(); assert(wifiOTA_submit("replacement", "new-password")); tick();
    boot(); assert(WiFi.target == "original"); assert(!wifiOTA_isTesting());
  } else if (scenario == "dhcp_required") {
    boot(); assert(wifiOTA_submit("home", "password")); tick();
    WiFi.link = WL_CONNECTED; WiFi.gotIp = false; tick(30000);
    assert(wifiOTA_requiresSetup()); assert(!testNvs::data.count("grova-wifi/credentials"));
  } else if (scenario == "first_save_failure") {
    boot(); assert(wifiOTA_submit("home", "password")); tick();
    testNvs::failKey = "grova-wifi/credentials"; connected();
    assert(wifiOTA_requiresSetup()); assert(wifiOTA_setupActive()); assert(restMode_isEnabled());
    assert(!testNvs::data.count("grova-wifi/credentials"));
  } else if (scenario == "corrupt_record") {
    saveOriginal(); testNvs::data["grova-wifi/credentials"][5] ^= 1;
    strcpy(testLegacySsid, "obsolete"); strcpy(testLegacyPassword, "old-password");
    boot(); assert(wifiOTA_requiresSetup()); assert(wifiOTA_setupActive());
    assert(std::string(wifiOTA_setupError()) == "saved_credentials_invalid");
  } else if (scenario == "recovery_without_run_reset") {
    saveOriginal(); tick(60000); restMode_setEnabled(false, true);
    WiFi.link = 0; tick(); tick(59999); assert(!wifiOTA_setupActive());
    tick(1); assert(wifiOTA_setupActive()); assert(!wifiOTA_requiresSetup()); assert(!restMode_isEnabled());
    connected(); tick(60000); assert(!wifiOTA_setupActive()); assert(!restMode_isEnabled());
  } else if (scenario == "input_and_scan") {
    boot(); assert(!wifiOTA_submit("", "password")); assert(!wifiOTA_submit("home", "short"));
    assert(!wifiCredentialsValid("123456789012345678901234567890123", "password"));
    assert(wifiCredentialsValid("home", ""));
    assert(wifiCredentialsValid("home", "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"));
    assert(wifiOTA_startScan()); assert(!wifiOTA_startScan());
    WiFi.scan = 0; assert(!wifiOTA_startScan()); tick(10000); assert(wifiOTA_startScan());
    assert(!wifiOTA_submit("home", "password")); WiFi.scan = 0;
    assert(wifiOTA_submit("home", "password")); assert(!wifiOTA_startScan());
  } else if (scenario == "millis_wrap") {
    clockMs = UINT32_MAX - 1000; boot(); assert(wifiOTA_submit("home", "password")); tick();
    tick(30000); assert(!wifiOTA_isTesting()); assert(wifiOTA_requiresSetup());
  } else if (scenario == "ap_failure") {
    WiFi.apFailure = true; boot(); assert(!wifiOTA_setupActive());
    assert(!wifiOTA_submit("home", "password")); WiFi.apFailure = false; tick();
    assert(!wifiOTA_setupActive()); tick(10000);
    assert(wifiOTA_setupActive());
  } else if (scenario == "key_failure_preserves_saved") {
    saveOriginal(); testNvs::data.erase("grova-wifi/setup-key");
    testNvs::failKey = "grova-wifi/setup-key";
    strcpy(testLegacySsid, "obsolete"); strcpy(testLegacyPassword, "old-password");
    boot(); assert(!wifiOTA_storageReady()); assert(!wifiOTA_requiresSetup());
    assert(WiFi.target == "original");
  } else if (scenario == "setup_key_write_failure") {
    testNvs::failKey = "grova-wifi/setup-key"; boot();
    assert(!wifiOTA_storageReady()); assert(!wifiOTA_submit("home", "password"));
    assert(wifiOTA_setupActive());
  } else assert(false && "unknown scenario");
  std::cout << scenario << " OK\n";
}
