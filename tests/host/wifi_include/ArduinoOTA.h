#pragma once
struct FakeOTA {
  int begins = 0;
  std::string hostname;
  void setHostname(const char* name) { hostname=name; }
  template<class T> void onStart(T) {}
  void begin() { ++begins; }
  void handle() {}
};
inline FakeOTA ArduinoOTA;
