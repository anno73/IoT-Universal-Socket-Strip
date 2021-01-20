#pragma once

#include <Arduino.h>

namespace relay {


extern void setup();
extern void loop();
extern void switchRelay(uint8_t index, uint8_t state);
extern void switchLed(uint8_t index, uint8_t state);

} // namespace relay