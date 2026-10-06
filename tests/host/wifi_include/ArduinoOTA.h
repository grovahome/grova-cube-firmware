#pragma once
struct FakeOTA {
  int begins = 0;
  void setHostname(const char*) {}
  template<class T> void onStart(T) {}
  void begin() { ++begins; }
  void handle() {}
};
inline FakeOTA ArduinoOTA;
