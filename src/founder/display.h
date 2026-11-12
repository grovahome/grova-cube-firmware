#pragma once
namespace founderDisplay {
void begin();
bool present();
bool configure(unsigned rotation,unsigned offset);
unsigned rotation();
unsigned offset();
void navigate(int delta);
void loop();
}  // namespace founderDisplay
