#include "controller/rtc.h"

#include <Wire.h>

#include "boards/grova_core_s3.h"
#include "controller/i2c_bus.h"
#include "controller/rtc_codec.h"
namespace founderRtc {
static bool found = false, backup = false;
static bool readReg(uint8_t reg, uint8_t* bytes, uint8_t count) {
  Wire.beginTransmission(founder::RTC_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(founder::RTC_ADDRESS, count) != count) {
    return false;
  }
  for (int i = 0; i < count; ++i) {
    bytes[i] = Wire.read();
  }
  return true;
}
static bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(founder::RTC_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}
bool begin() {
  founderBus::Scope bus;
  found = bool(bus) && founderBus::probe(founder::RTC_ADDRESS);
  backup = false;
  if (!found) {
    return false;
  }
  // Application Manual rev 1.3: keep RAM configuration from auto-refreshing,
  // enable level-switching backup, disable charger (battery chemistry unknown).
  uint8_t c1 = 0, pmu = 0, verify = 0;
  if (!readReg(0x10, &c1, 1) || !writeReg(0x10, c1 | 0x04) || !readReg(0xC0, &pmu, 1)) {
    return false;
  }
  pmu = (pmu & 0xC0) | 0x20;
  backup = writeReg(0xC0, pmu) && readReg(0xC0, &verify, 1) && verify == pmu;
  return backup;
}
bool read(uint32_t& utc) {
  founderBus::Scope bus;
  found = bool(bus) && founderBus::probe(founder::RTC_ADDRESS);
  if (!found) {
    return false;
  }
  uint8_t regs[7], status = 0, control = 0;
  return readReg(0x0D, &status, 1) && readReg(0x11, &control, 1) && readReg(0x01, regs, 7) &&
         rv3032::decode(regs, status, control, utc);
}
bool write(uint32_t utc) {
  uint8_t encoded[7];
  if (!rv3032::encode(utc, encoded)) {
    return false;
  }
  founderBus::Scope bus;
  found = bool(bus) && founderBus::probe(founder::RTC_ADDRESS);
  if (!found) {
    return false;
  }
  uint8_t control = 0;
  if (!readReg(0x11, &control, 1) || !writeReg(0x11, control | 1)) {
    return false;
  }
  Wire.beginTransmission(founder::RTC_ADDRESS);
  Wire.write(0x01);
  for (uint8_t value : encoded) {
    Wire.write(value);
  }
  bool ok = Wire.endTransmission() == 0;
  // On partial calendar writes leave STOP set: reboot must reject torn time.
  if (!ok) {
    return false;
  }
  if (!writeReg(0x11, control & ~1)) {
    return false;
  }
  uint8_t status = 0;
  if (!readReg(0x0D, &status, 1) || !writeReg(0x0D, status & ~3)) {
    return false;
  }
  uint32_t back = 0;
  // Already own the scope; validate readback without recursive bus locking.
  uint8_t regs[7];
  return readReg(0x0D, &status, 1) && readReg(0x11, &control, 1) && readReg(0x01, regs, 7) &&
         rv3032::decode(regs, status, control, back) && back >= utc && back <= utc + 1;
}
bool present() {
  return found;
}
bool backupConfigured() {
  return backup;
}
}  // namespace founderRtc
