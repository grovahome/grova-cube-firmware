// Production HTTP routes with transport/radio adapters replaced, no real sockets.
#include <cassert>
#include <iostream>
#include "modules/wifi_web.h"
#include "modules/wifi_ota.h"
#include "modules/rest_mode.h"
char testLegacySsid[33] = "", testLegacyPassword[65] = "";
static uint32_t clockMs = 0;
unsigned long millis() { return clockMs; }
void pump_inhibit() {}
int main() {
  wifiOTA_begin(); restMode_begin(); WebServer server; wifiWeb_register(server);
  assert(wifiWeb_setupRequest(server));
  server.request("/wifi", HTTP_GET); assert(server.code == 200);
  assert(server.responseHeaders["Cache-Control"] == "no-store");
  assert(std::string(server.body.c_str()).find(wifiOTA_setupPassword()) == std::string::npos);
  server.request("/api/v1/wifi", HTTP_GET); assert(server.code == 200);
  assert(std::string(server.body.c_str()).find(wifiOTA_setupPassword()) == std::string::npos);
  server.arguments["ssid"] = "home"; server.arguments["password"] = "private-password";
  for (int i = 0; i < 5; ++i) {
    server.request("/api/v1/wifi", HTTP_POST); assert(server.code == 403);
  }
  assert(!wifiOTA_isTesting());
  server.requestHeaders["X-GROVA-Setup-Key"] = wifiOTA_setupPassword();
  server.request("/api/v1/wifi", HTTP_POST); assert(server.code == 429);
  clockMs += 30000;
  server.request("/api/v1/wifi/scan", HTTP_POST); assert(server.code == 202);
  server.request("/api/v1/wifi", HTTP_POST); assert(server.code == 400);
  server.request("/api/v1/wifi/scan", HTTP_GET);
  assert(std::string(server.body.c_str()).find("\"scanning\":true") != std::string::npos);
  WiFi.scan = 1; server.request("/api/v1/wifi/scan", HTTP_GET);
  assert(std::string(server.body.c_str()).find("evil\\\"\\\\\\u000a<svg>") != std::string::npos);
  server.arguments["password"] = "short";
  server.request("/api/v1/wifi", HTTP_POST); assert(server.code == 400);
  server.arguments["password"] = "private-password";
  server.request("/api/v1/wifi", HTTP_POST); assert(server.code == 202);
  assert(wifiOTA_isTesting());
  server.request("/api/v1/wifi", HTTP_GET);
  assert(std::string(server.body.c_str()).find("private-password") == std::string::npos);
  wifiOTA_loop(); WiFi.link = WL_CONNECTED; ++clockMs; wifiOTA_loop(); clockMs += 2000; wifiOTA_loop();
  assert(!wifiWeb_setupRequest(server));
  server.localAddress = 2; assert(wifiWeb_setupRequest(server));
  server.requestHeaders.clear(); server.request("/api/v1/wifi/scan", HTTP_GET); assert(server.code == 403);
  std::cout << "Wi-Fi HTTP authorization, throttle, validation, scan escaping and interface isolation passed\n";
}
