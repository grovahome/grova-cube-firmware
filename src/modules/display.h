#pragma once

void display_begin();
void display_loop(float temp, float hum, int fanPercent, bool lightOn);
bool display_wakeFromUser();
