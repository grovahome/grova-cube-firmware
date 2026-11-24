#pragma once
namespace founderDisplay {
void begin();
const char* controller();
bool selectController(const char* name);
bool present();
bool configure(unsigned rotation, unsigned offset, const char* profile = nullptr);
unsigned rotation();
unsigned offset();
void navigate(int turn, bool pressed, bool held = false);
void loop();
}  // namespace founderDisplay
