#include "founder/web.h"

#include <WebServer.h>

#include "founder/app.h"
#include "founder/equipment.h"
#include "founder/history.h"
#include "founder/page.h"
#include "modules/wifi_web.h"
namespace founderWeb {
static WebServer server(80);
void begin() {
  wifiWeb_register(server);
  server.on("/", HTTP_GET, []() {
    if (wifiWeb_setupRequest(server)) {
      wifiWeb_page(server);
    } else {
      server.send_P(200, "text/html; charset=utf-8", FOUNDER_PAGE);
    }
  });
  auto status = []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", founderApp::status());
  };
  server.on("/api/v1/history", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", founderHistory::json());
  });
  server.on("/api/v1/configuration", HTTP_GET, []() {
    String output;
    server.sendHeader("Cache-Control", "no-store");
    if (founderEquipment::backup(output)) {
      server.send(200, "application/json", output);
    } else {
      server.send(503, "application/json",
                  "{\"ok\":false,\"message\":\"Configuration unavailable\"}");
    }
  });
  server.on("/api/v1/status", HTTP_GET, status);
  server.on("/api/status", HTTP_GET, status);
  server.on("/api/v1/control", HTTP_POST, []() {
    if (wifiWeb_setupRequest(server)) {
      server.send(403, "application/json",
                  "{\"ok\":false,\"message\":\"Setup network: controls disabled\"}");
      return;
    }
    if (server.header("Content-Type").indexOf("application/json") != 0) {
      server.send(415, "application/json", "{\"ok\":false,\"message\":\"Use application/json\"}");
      return;
    }
    String response;
    bool ok = founderApp::command(server.arg("plain"), response);
    server.send(ok ? 200 : 400, "application/json", response);
  });
  server.onNotFound([]() {
    if (wifiWeb_setupRequest(server) && server.method() == HTTP_GET) {
      wifiWeb_page(server);
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });
  server.begin();
}
void loop() {
  server.handleClient();
}
}  // namespace founderWeb
