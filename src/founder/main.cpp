#include <Arduino.h>

#include "founder/app.h"
#include "founder/devices.h"
#include "founder/display.h"
#include "founder/equipment.h"
#include "founder/i2c_bus.h"
#include "founder/inputs.h"
#include "founder/network.h"
#include "founder/outputs.h"
#include "founder/serial.h"
#include "founder/web.h"
#include "modules/fan.h"
#include "modules/rest_mode.h"
#include "modules/signal_registry.h"
#include "modules/time_sync.h"
#include "modules/wifi_ota.h"
void setup() {
  founderApp::bootSafetyBegin();  // Force GPIOs off and require release after every reboot.
  Serial.begin(115200);
  founderApp::settingsBegin();
  founderBus::begin();
  signalRegistry_begin();
  founderDevices::begin();
  founderInputs::begin();
  founderDisplay::begin();
  founderDisplay::loop();  // Show the first welcome frame before networking starts.
  wifiOTA_begin();
  time_begin();
  fan_begin();
  founderOutputs::begin();
  founderEquipment::begin();
  founderOutputs::interlock(true, wifiOTA_requiresSetup());
  founderNetwork::begin();
  founderWeb::begin();
  Serial.println("GROVA Founder application / Rest / configurable devices");
}
void loop() {
  wifiOTA_loop();
  bool setup = wifiOTA_requiresSetup() || wifiOTA_isTesting();
  if (setup || founderOutputs::inhibited()) {
    restMode_setEnabled(true, false);
  }
  founderOutputs::interlock(restMode_isEnabled(), setup);
  founderDevices::loop();
  time_loop();
  fan_loop();
  auto input = founderInputs::poll();
  if (input.requestSetup) {
    founderApp::setRest(true);
    wifiOTA_requestSetup();
  }
  founderDisplay::navigate(input.pageDelta);
  founderDisplay::loop();
  founderWeb::loop();
  founderSerial::loop();
  founderNetwork::loop();
  delay(5);
}
