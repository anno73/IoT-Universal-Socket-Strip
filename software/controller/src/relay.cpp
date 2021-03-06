#include <Arduino.h>
#include <Streaming.h>

#include <Wire.h>

#include <PolledTimeout.h>

#include "relay.h"

#include "../../relay/src/shared.h" // Shared data and command structures of relay

namespace relay
{

    /**
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
    const uint8_t INITIAL_IIC_ADDRESS = 127; // "Factory" default IIC address of an uninitialized relay module
    const uint8_t MAX_RELAY_MODULES = 20;    // Maximum number of supported relay modules. Mainly for sanity reasons on IIC bus scanning.
    const uint8_t MODULE_ADDR_OFFSET = 10;   // Start here with addresses for relay modules

    volatile uint8_t countSockets = 0; // Number of detected relay modules.

    /**
     * sendIicCommand
     * 
     * Send a single IIC command to relay module
     * 
     * 
     */
    uint8_t sendIicCommand(uint8_t address, uint8_t iicReg, int8_t value, uint8_t *status, uint8_t *tries)
    {
        *status = 255;
        *tries = 0;

        do
        {
            Wire.beginTransmission(address);
            Wire.write(iicReg);
            Wire.write(value);
            *status = Wire.endTransmission();

            // Serial << F("Write to address ") << address << F(" Status: ") << *status << F(" Try: ") << *tries << endl;
            if (*status != 0)
            {
                (*tries)++;
                delay(10);
            }
        } while ((*status != 0) && (*tries < MAX_IIC_RETRIES));

        // All went well
        if ((*status) == 0)
            return 0;

        // WARNING: we had to retry the operation
        if ((*tries) < MAX_IIC_RETRIES)
            return 1;

        // ERROR: Operation timed out: status != 0 and tries == MAX_IIC_RETRIES
        return 2;

    } // sendIicCommand

    /**
     * switchRelay
     * 
     */
    void switchRelay(uint8_t addr, uint8_t state)
    {
        // Serial << F("Switch relay ") << relay << F(" to state ") << ((state > 0) ? F("ON") : F("OFF")) << "." << endl;

        uint8_t status;
        uint8_t tries;
        uint32_t now = micros();

        addr += MODULE_ADDR_OFFSET;
        switch (sendIicCommand(addr, iicRegister::RELAY, state, &status, &tries))
        {
        // case 0:
        //     break;
        case 1:
            Serial << F("WARNING: Write to relay ") << addr << F(" Status: ") << status << F(" Try: ") << tries << endl;
            break;
        case 2:
            Serial << F("ERROR: Write to relay ") << addr << F(" Status: ") << status << F(" Try: ") << tries << endl;
            break;
            // default:
            //     break;
        }

        uint32_t diff = micros() - now;
        Serial << "switchRelay: diff " << diff << "us" << endl;

        return;
    } // switchRelay

    /**
     * switchLed
     * 
     */
    void switchLed(uint8_t addr, uint8_t state)
    {
        uint8_t status;
        uint8_t tries;

        addr += MODULE_ADDR_OFFSET;
        switch (sendIicCommand(addr, iicRegister::LED, state, &status, &tries))
        {
        case 1:
            Serial << F("WARNING: Write to led ") << addr << F(" Status: ") << status << F(" Try: ") << tries << endl;
            break;
        case 2:
            Serial << F("ERROR: Write to led ") << addr << F(" Status: ") << status << F(" Try: ") << tries << endl;
            break;
        default:
            break;
        }

        return;
    } // switchLed

    /**
     * getRegister
     * 
     * Read one register/byte from module
     * 
     */
    uint8_t getRegister(uint8_t module, uint8_t regIdx, uint8_t *data)
    {

        uint8_t status = 255;
        uint8_t tries = 0;

        uint8_t addr = module + MODULE_ADDR_OFFSET; // map module index to IIC address

        do
        {
            uint32_t now = micros();
            Wire.beginTransmission(addr); // Select register in module to read
            Wire.write(regIdx);
            status = Wire.endTransmission();
            Serial << F("getRegister: IIC call: ") << micros() - now << "us" << endl;

            if (status != 0)
            {
                tries++;
                delay(10);
            }
        } while ((status != 0) && (tries < MAX_IIC_RETRIES));

        if (tries >= MAX_IIC_RETRIES)
        {
            // there was an error
            return status;
        }

        // delay(1); // necessary?

        status = Wire.requestFrom(addr, (uint8_t)1, (uint8_t) true); // Request one byte from module; 3rd parm: true: send stop, false: send restart

        Serial << F("Wire.requestFrom returned ") << status << endl;

        if (status == 0)
        {
            Serial << F("getRegister: ERROR: did not receive any data") << endl;
            return 0x10;
        }

        // We got some data back
        if (Wire.available() == 1)
        {
            *data = Wire.read();
            return 0;
        }
        else
        {
            // We got more data back as expected
            Serial << F("getRegister: ERROR: expected bytes 1 but got ") << Wire.available() << endl;
            return 0x20;
        }

        return 0xA5;
    } // getRegister

    /**
     * setIicAddress
     * 
     */
    void setIicAddress(uint8_t currentAddr, uint8_t newAddr)
    {
        uint8_t status = 255;
        uint8_t tries = 0;

        switch (sendIicCommand(currentAddr, iicRegister::ADDR, newAddr, &status, &tries))
        {
        case 1:
            Serial << F("WARNING: Write to address ") << currentAddr << F(" Status: ") << status << F(" Try: ") << tries << endl;
            break;
        case 2:
            Serial << F("ERROR: Write to address ") << currentAddr << F(" Status: ") << status << F(" Try: ") << tries << endl;
            break;
        default:
            break;
        }

        return;
    } // setIicAddress

    /**
     * getModule
     * 
     * Read registers from relay module
     * 
     * 
     */
    uint8_t getModule(uint8_t idx, uint8_t *status, uint8_t *relay, uint8_t *led, uint8_t *addr, uint8_t *swVers)
    {
        // uint8_t response[5];
        // uint8_t res = 0;

        // if (idx > socketCnt) return 100;

        // res = readIicRegs(...);

        return 0;
    } // getModule

    /**
     * probeIicBusAddr
     * 
     * Probe IIC bus on a given address to see if a device is present.
     * 
     */
    uint8_t probeIicBusAddr(uint8_t addr)
    {
        Serial << F("probeIicBusAddr: ") << addr;

        uint8_t status = 255;
        uint8_t retryCount = 0;

        do
        {
            Wire.beginTransmission(addr);
            status = Wire.endTransmission();
            if (status != 0)
            {
                retryCount++;
                delay(10);
            }
        } while (status != 0 && retryCount <= MAX_IIC_RETRIES);

        Serial << F(" Status: ") << status << endl;

        return status;
    } // probeIicBusAddr

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
    void endlessToggleRelaisRaw(uint8_t address)
    {
        while (1)
        {
            Serial << F("I2C: Probing address ") << address << endl;

            Wire.beginTransmission(address);
            Wire.write(iicRegister::RELAY);
            Wire.write(relayCmd::TOGGLE);
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
    } // endlessToggleRelayRaw

    void timedToggleRelay(uint8_t address)
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
    } // timedToggleRelay

    void timedToggleRelayPolled(uint8_t address)
    {
        static esp8266::polledTimeout::periodicMs tick(5000);

        if (tick)
        {
            switchRelay(125, relayCmd::TOGGLE);
        }
    } // timedToggleRelayPolled

    /**
     * setup()
     * 
     */
    void setup(void)
    {

        Serial << F("Setup relay modules") << endl;

        // Setup IIC

        Wire.begin(PIN_SDA, PIN_SCL, 1);  // Init IIC with master address 1
        Wire.setClock(100000);            // 100kHz clock speed
        Wire.setClockStretchLimit(40000); // 40ms

        // scanIicBus(115, 127);     // Scan address range on IIC bus for existing devices and loop endlessly
        // endlessToggleRelais(125); // debugging IIC communication

        // Initialize and check for relay modules
        // do we get response on factory IIC address (what to use? 127? 0 is all call)? If so we want to start a initialization process.

        // Search for first free relay board address beginning at offset
        Serial << F("Probing for modules from address ") << MODULE_ADDR_OFFSET << " to " << MODULE_ADDR_OFFSET + MAX_RELAY_MODULES << endl;
        uint8_t freeAddr;
        for (freeAddr = MODULE_ADDR_OFFSET; freeAddr <= MAX_RELAY_MODULES; freeAddr++)
        {
            uint8_t status = probeIicBusAddr(freeAddr);
            // Status != 0 is an error, we most likely hit the first free address
            if (status)
                break;
        }

        countSockets = freeAddr - MODULE_ADDR_OFFSET;
        Serial << F("First free IIC address: ") << freeAddr << F(" Sockets found: ") << countSockets << endl;

        // Check if we have new devices on IIC bus and free IIC addresses left
        if (probeIicBusAddr(INITIAL_IIC_ADDRESS) == 0)
        {
            // Found a new device, need to initialize it with next free IIC address
            Serial << F("Found new device at ") << INITIAL_IIC_ADDRESS << endl;

            if (freeAddr)
            {
                Serial << F("Set new device's address to ") << freeAddr << endl;

                // Set module's new IIC address
                setIicAddress(INITIAL_IIC_ADDRESS, freeAddr);
            }
            else
            {
                Serial << F("WARNING: new device found but no free address left") << endl;
            }
        }

        return;
    } // setup

    /**
     * loop()
     * 
     */
    void loop(void)
    {
        // endlessToggleRelaisRaw(125);
        // timedToggleRelay(125);
        // timedToggleRelayPolled(125);

        return;
    } // loop

} // namespace relay