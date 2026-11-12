#pragma once
#include "Arduino.h"
#include <functional>
#define WIFI_STA 1
#define WIFI_AP_STA 3
#define WL_CONNECTED 3
#define WIFI_SCAN_RUNNING -1
#define WIFI_AUTH_OPEN 0
struct IPAddress {
  uint32_t value;
  explicit operator uint32_t() const { return value; }
  bool operator==(const IPAddress& other) const { return value == other.value; }
  String toString() const { return value ? "192.168.1.97" : "0.0.0.0"; }
};
struct FakeWiFi {
  int link = 0, scan = -2, beginCalls = 0;
  std::function<void()> beginHook;
  bool ap = false, apFailure = false, gotIp = true;
  std::string target, targetPassword;
  void persistent(bool) {}
  void mode(int) {}
  void setAutoReconnect(bool) {}
  bool softAP(const char*, const char*, int, int, int) { ap = !apFailure; return ap; }
  void softAPdisconnect(bool) { ap = false; }
  IPAddress softAPIP() { return {2}; }
  IPAddress localIP() { return {gotIp && link == WL_CONNECTED ? 1U : 0U}; }
  int status() { return link; }
  String SSID() { return String(target.c_str()); }
  String SSID(int) { return "evil\"\\\n<svg>"; }
  int RSSI(int) { return -65; }
  int encryptionType(int) { return 1; }
  void disconnect(bool, bool) { link = 0; }
  void begin(const char* ssid, const char* pass) { ++beginCalls; target = ssid; targetPassword = pass; if (beginHook) beginHook(); }
  void scanDelete() { scan = -2; }
  int scanComplete() { return scan; }
  int scanNetworks(bool, bool) { scan = WIFI_SCAN_RUNNING; return scan; }
};
inline FakeWiFi WiFi;
struct FakeESP { uint64_t mac=0x123456abcdefULL; uint64_t getEfuseMac() { return mac; } };
inline FakeESP ESP;
