#include <Arduino.h>
#include <Streaming.h>

#include <Wire.h>

#include "relay.h"

#include "../../relay/src/shared.h" // Shared data and command structures of relay

namespace relay
{

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

    const uint8_t MAX_IIC_RETRIES = 5;

    void switchRelay(uint8_t relay, uint8_t state)
    {
        // Serial << F("Switch relay ") << relay << F(" to state ") << ((state > 0) ? F("ON") : F("OFF")) << "." << endl;

        uint8_t status = 255;
        uint8_t tries = 0;

        do
        {
            Wire.beginTransmission(relay);
            Wire.write(iicRegister::RELAY);
            Wire.write(state);
            status = Wire.endTransmission();

            // Serial << F("Write to relay ") << relay << F(" Status: ") << status << F(" Try: ") << tries << endl;
            if (status != 0)
            {
                tries++;
                delay(10);
            }
        } while ((status != 0) && (tries <= MAX_IIC_RETRIES));

        if (status == 0 && tries != 0)
        {
            Serial << F("WARNING: Write to relay ") << relay << F(" Status: ") << status << F(" Try: ") << tries << endl;
        }

        if (status != 0)
        {
            Serial << F("ERROR: Write to relay ") << relay << F(" Status: ") << status << F(" Try: ") << tries << endl;
        }

        return;
    } // switchRelay

    /**
     * San IIC bus for devices and wait forever
     */
    void scanIicBus(uint8_t fromAddress, uint8_t toAddress)
    {

        for (uint8_t address = fromAddress; address <= toAddress; address++)
        {
            Serial << F("I2C: Probing address ") << address << endl;

            Wire.beginTransmission(address);
            uint8_t status = Wire.endTransmission();

            if (status == 0)
            {
                Serial << F("I2C: Device found at ") << address << F(".") << endl;
            }
            else
            {
                Serial << F("I2C: Possible error ") << status << F(" at address ") << address << endl;
            }
        }

        Serial << F("Sleep forever") << endl;
        while (1)
        {
            delay(1000);
            // yield();    // allow some ESP native background action (Wifi, NTP, ...)
        }
    } // scanIicBus

    /**
     *  Endlessly toggle relay on IIC address
     */
    void endlessToggleRelais(uint8_t address)
    {
        while (1)
        {
            Serial << F("I2C: Probing address ") << address << endl;

            Wire.beginTransmission(address);
            Wire.write(iicRegister::Relay);
            Wire.write(2);
            uint8_t status = Wire.endTransmission();

            if (status == 0)
            {
                Serial << F("I2C: Device found at ") << address << F(".") << endl;
            }
            else
            {
                Serial << F("I2C: Possible error ") << status << F(" at address ") << address << endl;
            }
            delay(1000);
            yield();
        }
    }   // endlessToggleRelay

    void setup(void)
    {

        Serial << F("Setup relay boards") << endl;

        // Setup IIC

        Wire.begin(PIN_SDA, PIN_SCL);

        // Initialize and check for socket controllers

        // scanIicBus(115, 127);

        // endlessToggleRelais(0);

        return;
    } // setup

    void loop(void)
    {
        static boolean relayState = false;
        static uint32_t relayTS = 0;

        uint32_t _nowTS = millis();

        if (_nowTS - relayTS > 1000)
        {
            relayTS = _nowTS;
            relayState = !relayState;
            switchRelay(125, relayState);
        }

        return;
    }

} // namespace relay