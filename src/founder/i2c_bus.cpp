#include "founder/i2c_bus.h"

#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "boards/grova_founder_s3.h"
namespace founderBus {
static SemaphoreHandle_t mutex = nullptr;
static bool mux = false;
static bool mask(uint8_t value) {
  Wire.beginTransmission(founder::MUX_ADDRESS);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}
bool probe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
void begin() {
  if (mutex) {
    return;
  }
  mutex = xSemaphoreCreateMutex();
  Wire.begin(founder::SDA, founder::SCL, 100000);
  Wire.setTimeOut(40);
  refresh();
}
void refresh() {
  if (!mutex || xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return;
  }
  mux = probe(founder::MUX_ADDRESS);
  if (mux) {
    mask(0);
  }
  xSemaphoreGive(mutex);
}
bool hasMux() {
  return mux;
}
Scope::Scope(int channel) {
  if (channel < -1 || channel > 7 || !mutex) {
    return;
  }
  locked_ = xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE;
  if (!locked_) {
    return;
  }
  if (mux) {
    ready_ = mask(channel < 0 ? 0 : uint8_t(1U << channel));
  } else {
    ready_ = channel == -1;
  }
}
Scope::~Scope() {
  if (locked_) {
    if (mux) {
      mask(0);
    }
    xSemaphoreGive(mutex);
  }
}
}  // namespace founderBus
