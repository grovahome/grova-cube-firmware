#pragma once
namespace founderInputs {
struct Event {
  int turn = 0;
  bool pressed = false;
  bool held = false;
  bool requestSetup = false;
};
void begin();
Event poll();
}  // namespace founderInputs
