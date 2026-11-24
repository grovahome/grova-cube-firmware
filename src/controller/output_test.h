#pragma once

namespace founderOutputTest {
constexpr unsigned DurationMs = 10000;
constexpr unsigned PortCount = 7;

bool available();
bool running();
unsigned remainingMs();
bool start(unsigned port, unsigned percent);
bool setLevel(unsigned percent);
void stop();
void loop();
}  // namespace founderOutputTest
