#include "controller/devices.h"

#include <Wire.h>

#include <cassert>
#include <iostream>

#include "controller/i2c_bus.h"
#include "modules/signal_registry.h"
static unsigned long nowMs = 100;
unsigned long millis() {
  return nowMs;
}
void delay(unsigned long ms) {
  nowMs += ms;
}
static void advance(unsigned ms) {
  unsigned long end = nowMs + ms;
  while (nowMs < end) {
    founderDevices::loop();
    nowMs += 5;
  }
}
int main() {
  founderBus::begin();
  signalRegistry_begin();
  Wire.sht[0] = Wire.sht[1] = true;
  founderDevices::begin();
  advance(4000);
  String inventory;
  founderDevices::appendJson(inventory);
  std::string text = inventory.c_str();
  assert(text.find("\"channel\":0") != std::string::npos &&
         text.find("\"channel\":1") != std::string::npos);
  auto first = signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C);
  assert(first.valid && std::string(first.source).find("ch0") != std::string::npos);
  assert(first.value > 24 && first.value < 26);
  assert(founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, first.source).valid);
  const std::string source = first.source;
  assert(text.find("\"signals\":") != std::string::npos);
  // Brief CRC failures retain the exact source and original sample timestamp.
  Wire.badCrc = true;
  advance(3000);
  auto held = founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, source.c_str());
  assert(held.valid && held.ageMs >= 3000 && held.ageMs < 10000);
  Wire.badCrc = false;
  advance(3000);
  assert(founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, source.c_str()).valid);
  // Loss does not immediately erase the sample or switch an exact-source rule.
  Wire.sht[0] = false;
  advance(3000);
  held = founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, source.c_str());
  assert(held.valid);
  // Boundary is measured from last success, not from detecting the failure.
  nowMs += 9999 - held.ageMs;
  assert(founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, source.c_str()).valid);
  ++nowMs;
  assert(!founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, source.c_str()).valid);
  advance(1000);
  assert(!founderDevices::readSignal(SIGNAL_CLIMATE_TEMPERATURE_C, source.c_str()).valid);
  auto fallback = signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C);
  assert(fallback.valid && std::string(fallback.source).find("ch1") != std::string::npos);
  Wire.badCrc = true;
  advance(14000);
  assert(!signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C).valid);
  Wire.badCrc = false;
  Wire.sht[0] = true;
  advance(14000);
  auto recovered = signalRegistry_read(SIGNAL_CLIMATE_TEMPERATURE_C);
  assert(recovered.valid && std::string(recovered.source).find("ch0") != std::string::npos);
  assert(Wire.mask == 0);
  std::cout << "Founder same-address sensors on separate channels, removal, CRC failure and "
               "recovery passed\n";
}
