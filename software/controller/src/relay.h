#pragma once

#include <Arduino.h>

namespace relay {

extern volatile uint8_t countSockets;

extern void setup();
extern void loop();
extern void switchRelay(uint8_t index, uint8_t state);
extern void switchLed(uint8_t index, uint8_t state);
extern uint8_t getRegister(uint8_t moduleIdx, uint8_t regIdx, uint8_t *data);

extern uint8_t getModule(uint8_t, uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *);

} // namespace relay