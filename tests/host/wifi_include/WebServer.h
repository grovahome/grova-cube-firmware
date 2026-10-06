#pragma once
#include "WiFi.h"
#include <functional>
#include <map>
#define HTTP_GET 0
#define HTTP_POST 1
class WebServer {
public:
  std::map<std::pair<std::string, int>, std::function<void()>> routes;
  std::map<std::string, String> arguments, requestHeaders, responseHeaders;
  int code = 0;
  String body;
  uint32_t localAddress = 1;
  struct Client { uint32_t address; IPAddress localIP() { return {address}; } };
  Client client() { return {localAddress}; }
  void collectHeaders(const char**, size_t) {}
  void on(const char* path, int method, std::function<void()> callback) { routes[{path, method}] = callback; }
  String arg(const char* name) { return arguments[name]; }
  String header(const char* name) { return requestHeaders[name]; }
  void sendHeader(const char* name, const char* value) { responseHeaders[name] = value; }
  void send(int status, const char*, const String& text) { code = status; body = text; }
  void send_P(int status, const char* type, const char* text) { send(status, type, text); }
  void request(const char* path, int method) { code = 0; body = ""; responseHeaders.clear(); routes.at({path, method})(); }
};
