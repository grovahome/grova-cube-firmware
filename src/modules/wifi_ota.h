#pragma once

void wifiOTA_begin();
void wifiOTA_loop();

bool wifiOTA_isConnected();
// Coordinator has initialized OTA and is servicing it on a validated WLAN.
bool wifiOTA_otaReady();
const char* wifiOTA_getIP();
bool wifiOTA_setupActive();
unsigned wifiOTA_setupCloseDelayMs();
bool wifiOTA_requiresSetup();
bool wifiOTA_isTesting();
bool wifiOTA_storageReady();
const char* wifiOTA_setupSSID();
const char* wifiOTA_setupIP();
const char* wifiOTA_setupPassword();
const char* wifiOTA_setupState();
const char* wifiOTA_setupError();
unsigned wifiOTA_disconnectReason();
int wifiOTA_beginResult();
const char* wifiOTA_savedSSID();
bool wifiOTA_submit(const char* ssid, const char* password);
bool wifiOTA_startScan();

// Open credential recovery without erasing the last validated network.
bool wifiOTA_requestSetup();
