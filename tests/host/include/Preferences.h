#pragma once
#include "Arduino.h"
#include <map>
#include <vector>
namespace testNvs {
inline std::map<std::string, std::vector<uint8_t>> data;
inline bool failWrites = false;
inline std::string failKey;
}
class Preferences {
  std::string name;
  std::string key(const char* k) const { return name + "/" + k; }
  template<class T> T read(const char* k, T fallback) {
    auto it = testNvs::data.find(key(k));
    if (it == testNvs::data.end() || it->second.size() != sizeof(T)) return fallback;
    T result; memcpy(&result, it->second.data(), sizeof(T)); return result;
  }
public:
  bool begin(const char* n, bool = false) { name = n; return true; }
  bool isKey(const char* k) { return testNvs::data.count(key(k)) != 0; }
  size_t getBytesLength(const char* k) { return isKey(k) ? testNvs::data[key(k)].size() : 0; }
  size_t getBytes(const char* k, void* output, size_t size) {
    if (!isKey(k) || getBytesLength(k) > size) return 0;
    const auto& bytes = testNvs::data[key(k)];
    memcpy(output, bytes.data(), bytes.size()); return bytes.size();
  }
  size_t putBytes(const char* k, const void* input, size_t size) {
    if (testNvs::failWrites || testNvs::failKey == key(k)) return 0;
    const auto* bytes = static_cast<const uint8_t*>(input);
    testNvs::data[key(k)] = std::vector<uint8_t>(bytes, bytes + size); return size;
  }
  size_t putBool(const char* k, bool value) { return putBytes(k, &value, sizeof(value)); }
  bool getBool(const char* k, bool value = false) { return read(k, value); }
  int8_t getChar(const char* k, int8_t value = 0) { return read(k, value); }
  int32_t getInt(const char* k, int32_t value = 0) { return read(k, value); }
  uint32_t getULong(const char* k, uint32_t value = 0) { return read(k, value); }
  uint32_t getUInt(const char* k, uint32_t value = 0) { return read(k, value); }
  String getString(const char* k, String value = String()) {
    return isKey(k) ? String(reinterpret_cast<const char*>(testNvs::data[key(k)].data())) : value;
  }
};
