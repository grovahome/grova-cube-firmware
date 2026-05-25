#pragma once

void wifiOTA_begin();
void wifiOTA_loop();

bool wifiOTA_isConnected();
const char* wifiOTA_getIP();
