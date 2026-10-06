#pragma once

void wifiOTA_begin();
void wifiOTA_loop();

bool wifiOTA_isConnected();
const char* wifiOTA_getIP();
bool wifiOTA_setupActive();
bool wifiOTA_requiresSetup();
bool wifiOTA_isTesting();
bool wifiOTA_storageReady();
const char* wifiOTA_setupSSID();
const char* wifiOTA_setupIP();
const char* wifiOTA_setupPassword();
const char* wifiOTA_setupState();
const char* wifiOTA_setupError();
const char* wifiOTA_savedSSID();
bool wifiOTA_submit(const char* ssid, const char* password);
bool wifiOTA_startScan();
