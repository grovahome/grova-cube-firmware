#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
struct WifiCredentials {
  uint32_t version;
  char ssid[33];
  char password[65];
  uint32_t checksum;
};
inline bool wifiCredentialsValid(const char* ssid, const char* password) {
  const size_t s = strlen(ssid), p = strlen(password);
  if (!s || s > 32) return false;
  if (p == 64) {
    for (size_t i = 0; i < p; ++i) {
      const char c = password[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
  }
  return p == 0 || (p >= 8 && p <= 63);
}
inline uint32_t wifiCredentialsChecksum(const WifiCredentials& c) {
  uint32_t hash = 2166136261UL;
  const auto* bytes = reinterpret_cast<const uint8_t*>(&c);
  // Exclude compiler padding between password and checksum.
  for (size_t i = 0; i < offsetof(WifiCredentials, password) + sizeof(c.password); ++i) hash = (hash ^ bytes[i]) * 16777619UL;
  return hash;
}
inline bool wifiCredentialsRecordValid(const WifiCredentials& c) {
  return c.version == 1 && memchr(c.ssid, 0, sizeof(c.ssid)) &&
    memchr(c.password, 0, sizeof(c.password)) && wifiCredentialsValid(c.ssid, c.password) &&
    c.checksum == wifiCredentialsChecksum(c);
}
