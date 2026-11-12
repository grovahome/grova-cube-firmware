#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_system.h>
#include <string.h>
#include "config.h"
#include "modules/wifi_ota.h"
#include "modules/wifi_credentials.h"
#if defined(GROVA_FOUNDER_APP)
#include "founder/outputs.h"
#include "device_identity.h"
#include "modules/fan.h"
#else
#include "modules/pump.h"
#endif
#include "modules/rest_mode.h"

static Preferences settings;
static DNSServer dns;
static WifiCredentials current = {}, candidate = {};
static bool storageReady = false, haveCurrent = false, needsSetup = true;
static bool apOpen = false, otaReady = false, trial = false, trialQueued = false;
static bool stable = false, closeScheduled = false;
static uint32_t attemptAt = 0, offlineAt = 0, stableAt = 0, closeAt = 0, scanAt = 0;
static bool wasConnected = false, scanned = false, apAttempted = false;
static uint32_t apAttemptAt = 0;
static char setupName[20] = "", setupKey[13] = "";
static String ipText = "-";
static String setupIpText = "-";
static const char* state = "starting";
static const char* error = "";

static void connectTo(const WifiCredentials& credentials) {
  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  WiFi.begin(credentials.ssid, credentials.password);
  attemptAt = millis();
  stable = false;
}

static bool openSetup() {
  if (apOpen) return true;
  if (apAttempted && millis() - apAttemptAt < 10000UL) return false;
  apAttempted = true; apAttemptAt = millis();
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(setupName, setupKey, 1, 0, 2)) {
    error = "setup_ap_failed";
    return false;
  }
  apOpen = true;
  setupIpText = WiFi.softAPIP().toString();
  dns.start(53, "*", WiFi.softAPIP());
  closeScheduled = false;
  return true;
}

static void failTrial(const char* reason) {
  error = reason; state = "failed";
  trial = trialQueued = stable = false;
  memset(&candidate, 0, sizeof(candidate));
  if (haveCurrent) connectTo(current);
  else WiFi.disconnect(false, false);
}

void wifiOTA_begin() {
  current = {}; candidate = {};
  haveCurrent = apOpen = otaReady = trial = trialQueued = stable = closeScheduled = false;
  wasConnected = scanned = apAttempted = false;
  error = ""; state = "starting"; ipText = "-";
  storageReady = settings.begin("grova-wifi", false);
  const bool storageReadable = storageReady;
  WiFi.persistent(false); // Only validated credential blobs own persistence.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
#if defined(GROVA_FOUNDER_APP)
  char suffix[13];
  grovaMacSuffix(ESP.getEfuseMac(), suffix);
  snprintf(setupName, sizeof(setupName), "GROVA-%s", suffix);
#else
  snprintf(setupName, sizeof(setupName), "GROVA-%06lX", static_cast<unsigned long>(ESP.getEfuseMac() & 0xffffff));
#endif
  memset(setupKey, 0, sizeof(setupKey));
  bool keyValid = storageReady && settings.getBytesLength("setup-key") == sizeof(setupKey) &&
    settings.getBytes("setup-key", setupKey, sizeof(setupKey)) == sizeof(setupKey) && setupKey[12] == 0;
  const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  for (unsigned i = 0; i < 12 && keyValid; ++i) keyValid = setupKey[i] && strchr(alphabet, setupKey[i]);
  if (!keyValid) {
    uint8_t random[12]; esp_fill_random(random, sizeof(random));
    for (unsigned i = 0; i < 12; ++i) setupKey[i] = alphabet[random[i] % (sizeof(alphabet) - 1)];
    setupKey[12] = 0;
    if (!storageReady || settings.putBytes("setup-key", setupKey, sizeof(setupKey)) != sizeof(setupKey)) {
      storageReady = false; error = "storage_unavailable";
    }
  }
  if (storageReadable && settings.isKey("credentials")) {
    haveCurrent = settings.getBytesLength("credentials") == sizeof(current) &&
      settings.getBytes("credentials", &current, sizeof(current)) == sizeof(current) && wifiCredentialsRecordValid(current);
    if (!haveCurrent) { current = {}; error = "saved_credentials_invalid"; }
  } else if (wifiCredentialsValid(WIFI_SSID, WIFI_PASS)) {
    // Existing devices retain their compiled network until a web setup succeeds.
    current.version = 1;
    strcpy(current.ssid, WIFI_SSID); strcpy(current.password, WIFI_PASS);
    current.checksum = wifiCredentialsChecksum(current);
    haveCurrent = true;
  }
  needsSetup = !haveCurrent;
  offlineAt = attemptAt = millis();
  if (haveCurrent) { state = "connecting"; connectTo(current); }
  else { state = "setup"; openSetup(); }
}

void wifiOTA_loop() {
  const uint32_t now = millis();
  if (apOpen) dns.processNextRequest();
  if (trialQueued) { trialQueued = false; connectTo(candidate); }
  const bool connected = WiFi.status() == WL_CONNECTED && static_cast<uint32_t>(WiFi.localIP()) != 0;
  if (trial) {
    if (connected && WiFi.SSID() == candidate.ssid) {
      if (!stable) { stable = true; stableAt = now; }
      if (now - stableAt >= 2000UL) {
        if (needsSetup && !restMode_setEnabled(true, true)) failTrial("rest_storage_failed");
        else if (!storageReady || settings.putBytes("credentials", &candidate, sizeof(candidate)) != sizeof(candidate)) failTrial("storage_failed");
        else {
          current = candidate; memset(&candidate, 0, sizeof(candidate));
          haveCurrent = true; needsSetup = false; trial = false;
          state = "saved"; error = "";
        }
      }
    } else stable = false;
    if (trial && now - attemptAt >= 30000UL) failTrial("connection_failed");
  }
  const bool online = WiFi.status() == WL_CONNECTED && static_cast<uint32_t>(WiFi.localIP()) != 0;
  if (online && !trial) {
    ipText = WiFi.localIP().toString();
    if (strcmp(state, "failed") && strcmp(state, "saved")) state = "connected";
    if (!otaReady && !needsSetup) {
#if defined(GROVA_FOUNDER_APP)
      ArduinoOTA.setHostname(setupName);
#else
      ArduinoOTA.setHostname(OTA_HOSTNAME);
#endif
      ArduinoOTA.onStart([]() {
#if defined(GROVA_FOUNDER_APP)
        founderOutputs::inhibit(); fan_forceOff(); restMode_setEnabled(true,false);
#else
        pump_inhibit();
#endif
      });
      ArduinoOTA.begin(); otaReady = true;
    }
    if (otaReady) ArduinoOTA.handle();
    if (apOpen) {
      if (!closeScheduled) { closeScheduled = true; closeAt = now; }
      if (now - closeAt >= 60000UL) {
        dns.stop(); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA);
        apOpen = false; closeScheduled = false;
      }
    }
  } else {
    ipText = "-"; closeScheduled = false;
    if (wasConnected) offlineAt = now;
    if (!apOpen && (needsSetup || now - offlineAt >= 60000UL)) openSetup();
    if (!trial && haveCurrent && WiFi.scanComplete() != WIFI_SCAN_RUNNING && now - attemptAt >= 30000UL) connectTo(current);
  }
  wasConnected = online;
}

bool wifiOTA_isConnected() { return WiFi.status() == WL_CONNECTED && static_cast<uint32_t>(WiFi.localIP()) != 0; }
const char* wifiOTA_getIP() { return ipText.c_str(); }
bool wifiOTA_setupActive() { return apOpen; }
bool wifiOTA_requiresSetup() { return needsSetup; }
bool wifiOTA_isTesting() { return trial; }
bool wifiOTA_storageReady() { return storageReady; }
const char* wifiOTA_setupSSID() { return setupName; }
const char* wifiOTA_setupIP() { return setupIpText.c_str(); }
const char* wifiOTA_setupPassword() { return setupKey; }
const char* wifiOTA_setupState() { return state; }
const char* wifiOTA_setupError() { return error; }
const char* wifiOTA_savedSSID() { return haveCurrent ? current.ssid : ""; }

bool wifiOTA_submit(const char* ssid, const char* password) {
  if (trial || WiFi.scanComplete() == WIFI_SCAN_RUNNING || !storageReady || !wifiCredentialsValid(ssid, password)) return false;
  candidate = {}; candidate.version = 1;
  strcpy(candidate.ssid, ssid); strcpy(candidate.password, password);
  candidate.checksum = wifiCredentialsChecksum(candidate);
  if (!openSetup()) { memset(&candidate, 0, sizeof(candidate)); return false; }
  trial = trialQueued = true; closeScheduled = false;
  state = "testing"; error = "";
  return true;
}

bool wifiOTA_startScan() {
  const uint32_t now = millis();
  if (trial || WiFi.scanComplete() == WIFI_SCAN_RUNNING || (scanned && now - scanAt < 10000UL)) return false;
  WiFi.scanDelete(); scanAt = now; scanned = true;
  const int result = WiFi.scanNetworks(true, true);
  return result == WIFI_SCAN_RUNNING || result >= 0;
}

bool wifiOTA_requestSetup() { return openSetup(); }
