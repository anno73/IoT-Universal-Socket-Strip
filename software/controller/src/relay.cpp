#include <Arduino.h>
#include <Streaming.h>

#include "relay.h"

/**
 * 
 * I2C
 * 
 * Default I2C pins on ESP8266
 * D1   SDA
 * D2   SCL
 * 
 */
const uint8_t PIN_SDA = D1;
const uint8_t PIN_SCL = D2;

namespace relay {

void switchRelay(uint8_t relay, uint8_t state)
{
    Serial << F("Switch relay ") << relay << F("to state ") << ((state > 0) ? F("ON") : F("OFF")) << "." << endl;
    return;
}   // Relay



void setup(void) {

    Serial << F("Setup relay boards") << endl;

    // Setup IIC



    // Initialize and check for socket controllers

    return;

}   // setup

void loop(void) {
    return;
}

}   // namespace relay