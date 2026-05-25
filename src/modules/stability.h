#pragma once

void stability_begin();
void stability_loop();

bool isSystemHealthy();

float safeTemp(float t);
float safeHum(float h);