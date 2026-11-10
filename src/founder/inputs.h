#pragma once
namespace founderInputs {
struct Event {
  int pageDelta = 0;
  bool requestSetup = false;
};
void begin();
Event poll();
}  // namespace founderInputs
