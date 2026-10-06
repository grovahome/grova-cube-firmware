#pragma once
#include "WiFi.h"
struct DNSServer {
  bool start(int, const char*, IPAddress) { return true; }
  void stop() {}
  void processNextRequest() {}
};
