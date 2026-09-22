#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
using std::min;
using std::max;
unsigned long millis();
class String {
  std::string value;
public:
  String(const char* text = "") : value(text) {}
  String(float number, int decimals) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed); stream.precision(decimals); stream << number;
    value = stream.str();
  }
  const char* c_str() const { return value.c_str(); }
  String& operator+=(const char* text) { value += text; return *this; }
  String& operator+=(char text) { value += text; return *this; }
  String& operator+=(const String& text) { value += text.value; return *this; }
  template<class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
  String& operator+=(T number) { value += std::to_string(number); return *this; }
};
struct TestSerial {
  template<class T> void print(const T&) {}
  template<class T> void println(const T&) {}
};
inline TestSerial Serial;
