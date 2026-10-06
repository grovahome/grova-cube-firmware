#pragma once
#include <WebServer.h>
void wifiWeb_register(WebServer& server);
void wifiWeb_page(WebServer& server);
bool wifiWeb_setupRequest(WebServer& server);
