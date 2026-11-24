#include <Arduino.h>

#include <array>
#include <cassert>
#include <iostream>

#include "boards/grova_core_s3.h"
#include "controller/inputs.h"

static uint32_t now = 1000;
static std::array<int, 49> pins;
static std::array<void (*)(), 49> interrupts{};

unsigned long millis() {
  return now;
}

int digitalRead(int pin) {
  return pins[pin];
}

void pinMode(int, int) {}

void attachInterrupt(int pin, void (*handler)(), int mode) {
  assert(mode == CHANGE);
  interrupts[pin] = handler;
}

static void phase(int pair) {
  const int clock = pair >> 1;
  const int data = pair & 1;
  const bool clockChanged = pins[founder::ENC_CLK] != clock;
  const bool dataChanged = pins[founder::ENC_DT] != data;
  pins[founder::ENC_CLK] = clock;
  pins[founder::ENC_DT] = data;
  if (clockChanged) interrupts[founder::ENC_CLK]();
  if (dataChanged) interrupts[founder::ENC_DT]();
}

static void clockwise() {
  for (int pair : {1, 0, 2, 3}) phase(pair);
}

static void counterclockwise() {
  for (int pair : {2, 0, 1, 3}) phase(pair);
}

int main() {
  pins.fill(HIGH);
  founderInputs::begin();
  for (int pair : {1, 3, 1, 3}) phase(pair);
  assert(founderInputs::poll().turn == 0);  // Contact bounce.
  for (int pair : {1, 0, 1, 3}) phase(pair);
  assert(founderInputs::poll().turn == 0);  // Reversal before a detent.
  for (int pair : {1, 2, 3}) phase(pair);
  assert(founderInputs::poll().turn == 0);  // Missed edge.

  for (int pair : {1, 3, 1, 0, 1, 0, 2, 3}) phase(pair);
  assert(founderInputs::poll().turn == 1);  // One full detent despite bounce.
  clockwise();
  assert(founderInputs::poll().turn == 0);  // Suppress a very fast follow-up.
  now += 150;
  assert(founderInputs::poll().turn == 0);  // No delayed follow-up.

  counterclockwise();
  counterclockwise();
  assert(founderInputs::poll().turn == -1);  // Several detents while drawing.
  assert(founderInputs::poll().turn == 0);
  now += 150;
  clockwise();
  assert(founderInputs::poll().turn == 1);  // Another deliberate detent.
  now += 150;
  pins[founder::ENC_SW] = LOW;
  clockwise();
  assert(founderInputs::poll().turn == 0);  // Moving while pressing.
  now += 80;
  clockwise();
  pins[founder::ENC_SW] = HIGH;
  auto click = founderInputs::poll();
  assert(click.pressed && !click.held && click.turn == 0);
  pins[founder::ENC_SW] = LOW;
  founderInputs::poll();
  now += 800;
  pins[founder::ENC_SW] = HIGH;
  auto hold = founderInputs::poll();
  assert(hold.held && !hold.pressed && hold.turn == 0);
  std::cout << "Encoder detents, bounce, reversal, input pacing and button passed\n";
}
