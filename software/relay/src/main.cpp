#include <Arduino.h>

// #include <avr/sleep.h>
// #include <avr/wdt.h>
// #include <avr/power.h>

#include "shared.h"

// #define DEBUG

#ifdef DEBUG
    #warning DEBUG enabled on target. SoftwareSerial output on LED pin.
    const unsigned long txOnlySerialBaudRate = 115200;
    #include <SendOnlySoftwareSerial.h> // https://github.com/nickgammon/SendOnlySoftwareSerial
    #include <Streaming.h>              // https://github.com/janelia-arduino/Streaming

    #define dbg(x) txOnlySerial << x
#else
    #define dbg(x)
#endif

#include "EEPROM.h"
#include <EEPROMWearLevel.h> // https://github.com/PRosenb/EEPROMWearLevel

// const byte EWL_LAYOUT_VERSION = 1;
#define EWL_LAYOUT_VERSION 1
// const int EWL_USE_BYTES = 500;
#define EWL_USE_BYTES 500

const int EWLIDX_STATUS = 0;
const int EWLIDX_RELAY = 1;
const int EWLIDX_LED = 2;
// const int EWLIDX_BUTTON = 3;
// const int EWLIDX_COUNT = 3;
#define EWLIDX_COUNT 3

const unsigned int EEPIDX_IICADDR = 511; // Absolute EEPROM index

#include <TinyWireS.h> // https://github.com/nickcengel/TinyWireSio
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE (16)
#endif

void dumpEEPROM(void);

/**
 * ATTiny85 Pin Usage
 * 
 * ATTiny   Port  Arduino   Function
 * Pin #    Pin   Pin # 
 *   1      PB5    5/A0     /RESET
 *   2      PB3    3/A3
 *   3      PB4    4/A2
 *   4      -       -       GND
 *   5      PB0     0       IIC SDA
 *   6      PB1     1
 *   7      PB2    2/A1     IIC SCL
 *   8              -       VCC
 */

const uint8_t PIN_SCL = 0;
const uint8_t PIN_SDA = 2;

const uint8_t PIN_RL1 = 1;
const uint8_t PIN_RL2 = 3;

const uint8_t PIN_LED = 4;
const uint8_t PIN_BUTTON = 5;

const uint32_t IIC_BITRATE = 100000;
volatile uint8_t iicSlaveAddress = 125;
uint8_t softwareVersion = 0;

// const uint8_t RELAY_SWITCH_PULSE_DURATION_MS = 15;
const uint8_t RELAY_SWITCH_PULSE_DURATION_MS = 50;
// const uint8_t RELAY_COIL_OFF = LOW;
// const uint8_t RELAY_COIL_ON = HIGH;
const uint8_t RELAY_COIL_OFF = HIGH;
const uint8_t RELAY_COIL_ON = LOW;

const uint8_t LED_ON = LOW;
const uint8_t LED_OFF = HIGH;

#ifdef DEBUG
SendOnlySoftwareSerial txOnlySerial(PIN_LED);   // Reuse LED pin for software serial.
#endif

/*
  Operations !!! BEWARE OF FEATURE CREEP !!! Maybe creep a bit for education and fun.

  Button is totally optional due to mechanical reasons of base hardware.

  Relay operation
    Set relay state ON/OFF
    Get relay state
    Set freeze relay state lock/unlock 
    Get freeze relay state
  Set LED operational mode
    0: Follow relay (default)
    1: -
    2: Independent fixed state
      0: off
      1: on
    4: Independent blink
      3..0: rate
  Get LED status
  Get LED operational mode
  Set button operation
    0: Toggle relay
    1: off
    2: notify I2C master
  Get button status
  Set IIC Address
  Get IIC Address
  Get Software Version
*/

/**
 * 
 * ??? have array or dedicated variables to hold I2C register values
 * ??? we could have "virtual" registers not needing a storage space because they trigger some action upon r/w.
 *
 * Usage would probably be like this then:
 * typedef struct { uint8_t a, b, c; } reg_struct; 
 * union iicReg_u { uint8_t r[6] ; reg_struct s; }
 * iicReg_u iicRegs;
 * iicReg.r[0] = 0;
 * iicReg.s.a = 0;
 * 
 */

// Make this a union of array and struct
// volatile uint8_t iicRegs[] = {
//     0x00, // 0: Status Register
//     0x00, // 1: Set relay state
//     0x00, // 2: IIC Address Register
//     0x00  // 3: Software version
// };

// For now be conservative and define each possible register on its own. Select access via switch/case instead of array index.

/**
 * statusReg
 * 
 * 76543210
 * 
 * 0: relay status
 *      0: off
 *      1: on
 * 2: led status
 *      0: off
 *      1: on
 */
volatile uint8_t statusReg = 0;

/**
 * relayReg
 * 
 * 76543210
 * 
 * 0:   on/off
 *      0: relay off
 *      1: relay on
 * 1:   freeze/unfreeze
 *      0: unfreeze
 *      1: freeze
 */
volatile uint8_t relayReg = 0;

/**
 * ledReg
 * 
 * 76543210
 * 
 * 0: on/off
 *      0: off
 *      1: on
 * 1:   follow relay
 *      0: follow relay
 *      1: switch separately
 * 2:   freeze/unfreeze
 *      0: unfreeze
 *      1: freeze
 * 3:
 * 54:  blink rate
 *      00
 *      01
 *      10
 *      11
 */
volatile uint8_t ledReg = 0;

/**
 * buttonReg
 * 
 * 76543210
 * 
 * 0: on/off    Read only
 *      0: off
 *      1: on
 * 1: enable/disable
 *      0: enabled
 *      1: disabled
 * 2: freeze/unfreeze
 *      0: unfreeze
 *      1: freeze
 */
volatile uint8_t buttonReg = 0;
// volatile uint8_t addrReg = 0;    // does not need to be a physical register. Is obtained from EEPROM.
// volatile uint8_t versionReg = 0; // does not need to be a physical register. Is a constant in the code.

volatile uint8_t iicRegSelected;                  // For read operations persist the selected register across IIC operations
const uint8_t iicRegSize = iicRegister::REGCOUNT; // How many registers we have defined
const uint8_t iicRegLastIdx = iicRegSize - 1;     // Index of the last register for wrap around on multiple read/write operations

volatile bool persistIicRegs; // If true, persist register values in main loop

/**
 * 
 * ledAction
 * 
 */
void ledAction(uint8_t action)
{

    dbg(F("ledAction: ") << action << endl);

    if (bitRead(ledReg, 1) == 0)
    { // LED in follow relay mode. Only some actions allowed:
        switch (action)
        {
        case ledCmd::UNFOLLOW_RELAY: // intentionally fallthrough all allowed commands.
        case ledCmd::FOLLOW_RELAY:
        case ledCmd::FREEZE:
        case ledCmd::UNFREEZE:
        case ledCmd::RELAY_OFF:
        case ledCmd::RELAY_ON:
            break;
        default:
            return;
        }
    }

    if (bitRead(ledReg, 2) == 1)
    { // LED in freeze mode. Only some actions allowed:
        switch (action)
        {
        case ledCmd::RELAY_OFF: // intentionally fallthrough all allowed commands.
        case ledCmd::RELAY_ON:
        case ledCmd::FREEZE:
        case ledCmd::UNFREEZE:
            break;
        default:
            return;
        }
        return;
    }

    persistIicRegs = true;

    switch (action)
    {
    case ledCmd::RELAY_OFF:
        bitClear(ledReg, 0);
        bitClear(statusReg, 2);

#ifndef DEBUG
        digitalWrite(PIN_LED, LED_OFF);
#endif
        break;
    case ledCmd::RELAY_ON:
        bitSet(ledReg, 0);
        bitSet(statusReg, 2);

#ifndef DEBUG
        digitalWrite(PIN_LED, LED_ON);
#endif
        break;
    case ledCmd::UNFOLLOW_RELAY:
        bitClear(ledReg, 1);
        break;
    case ledCmd::FOLLOW_RELAY:
        bitSet(ledReg, 1);

        // Initialize LED with current relay state
        if (bitRead(relayReg, 0) == 0)
        {
            ledAction(ledCmd::RELAY_OFF);
        }
        else
        {
            ledAction(ledCmd::RELAY_ON);
        }
        break;
    case ledCmd::OFF:
        bitClear(ledReg, 0);
        bitClear(statusReg, 0);

#ifndef DEBUG
        digitalWrite(PIN_LED, LED_OFF);
#endif
        break;
    case ledCmd::ON:
        bitSet(ledReg, 0);
        bitSet(statusReg, 2);

#ifndef DEBUG
        digitalWrite(PIN_LED, LED_ON);
#endif
        break;
    case ledCmd::TOGGLE:
        if (bitRead(ledReg, 0) == 0)
        {
            ledAction(ledCmd::ON);
        }
        else
        {
            ledAction(ledCmd::OFF);
        }
        break;
    case ledCmd::UNFREEZE:
        bitClear(ledReg, 2);
        break;
    case ledCmd::FREEZE:
        bitSet(ledReg, 2);
        break;
    default:
        // Unknown or unimplemented command
        persistIicRegs = false;
        break;
    }

    return;
} // ledAction

/**
 * relayAction
 * 
 * Initial idea:
 * Execute operations that take longer than allowed in the IIC callback routine.
 * This is mainly the relay switching, as it requires some delay on the coil to properly switch.
 * 
 * Now:
 * Encapsulate relay operation
 */
void relayAction(uint8_t action)
{

    dbg(F("relayAction: ") << action << endl);

    // If relay is in freeze state then only action allowed is unfreeze
    if (bitRead(relayReg, 1) && (action != relayCmd::UNFREEZE))
    {
        return;
    }

    persistIicRegs = true;

    switch (action)
    {
    case relayCmd::OFF:
        // if (bitRead(relayReg, 0) == 0)
        // break;

        // iicRegs[IIC_REG_STATUS] &= ~1;
        bitClear(statusReg, 0);
        bitClear(relayReg, 0);

        digitalWrite(PIN_RL1, RELAY_COIL_ON);
        tws_delay(RELAY_SWITCH_PULSE_DURATION_MS);
        digitalWrite(PIN_RL1, RELAY_COIL_OFF);

        // digitalWrite(PIN_LED, LED_OFF);
        ledAction(ledCmd::RELAY_OFF);
        break;
    case relayCmd::ON:
        // iicRegs[IIC_REG_STATUS] |= 1;
        bitSet(statusReg, 0);
        bitSet(relayReg, 0);

        digitalWrite(PIN_RL2, RELAY_COIL_ON);
        tws_delay(RELAY_SWITCH_PULSE_DURATION_MS);
        digitalWrite(PIN_RL2, RELAY_COIL_OFF);

        // digitalWrite(PIN_LED, LED_ON);
        ledAction(ledCmd::RELAY_ON);
        break;
    case relayCmd::TOGGLE:
        if (relayReg & 1)
        {
            relayAction(relayCmd::OFF);
        }
        else
        {
            relayAction(relayCmd::ON);
        }
        break;
    case relayCmd::FREEZE:
        bitSet(relayReg, 1);
        break;
    case relayCmd::UNFREEZE:
        bitClear(relayReg, 1);
        break;
    default:
        // Unknown or unimplemented command
        persistIicRegs = false;
        break;
    }

    return;
} // relayAction

/**
 *  iicRequestEventCb
 * 
 * This is called for each read request we receive.
 * Never put more than one byte of data (with TinyWireS.send) to the send-buffer when using this callback.
 * 
 */
void iicRequestEventCb()
{
    // uint8_t tmp;

    dbg(F("iicRequestEventCb for reg ") << iicRegSelected << endl);

    switch (iicRegSelected)
    {
    case iicRegister::STATUS:
        TinyWireS.send(statusReg);
        break;
    case iicRegister::RELAY:
        TinyWireS.send(relayReg);
        break;
    case iicRegister::LED:
        TinyWireS.send(ledReg);
        break;
    case iicRegister::BUTTON:
        TinyWireS.send(buttonReg);
        break;
    case iicRegister::ADDR:
        TinyWireS.send(iicSlaveAddress);
        break;
    case iicRegister::VERSION:
        TinyWireS.send(softwareVersion);
        break;
    default:
        break;
    }

    // Increment the reg position on each read, and loop back to zero
    iicRegSelected++;

    // Treat registers as ring buffer. Check for possible wrap around and handle it.
    if (iicRegSelected >= iicRegLastIdx)
    {
        iicRegSelected = 0;
    }

} // iicRequestEventCb

/**
 * 
 * iicReceiveEventCb
 * 
 * The I2C data received handler triggers actions in our little device.
 *
 * This needs to complete before the next incoming transaction (start, data, restart/stop) on the bus does
 * so be quick, set flags for long running tasks to be called from the mainloop instead of running them directly
 * 
 */
void iicReceiveEventCb(uint8_t howMany)
{

    dbg(F("iicReceiveEventCb") << endl);

    if (howMany < 1)
        // Sanity check
        return;

    if (howMany > TWI_RX_BUFFER_SIZE)
        // Sanity check - should no occur really
        return;

    iicRegSelected = TinyWireS.receive();

    if (iicRegSelected > iicRegLastIdx)
        // Sanity check - non existant register requested
        return;

    howMany--;

    if (!howMany)
        // This write was only to select the register for next read operation (iicRequestEvent)
        return;

    // Seems we got a valid write request with at least another byte of data
    // iicRegSelected indicates what register to start with as subsequent received bytes advance to the next register

    while (howMany--)
    {
        uint8_t data = TinyWireS.receive();

        dbg(F("iicRecCb: Reg: ") << iicRegSelected << " Data: " << data << endl);

        switch (iicRegSelected)
        {
            // We only allow operation on one attribute at a time

        // case iicRegister::STATUS:
        // Ignore writes to STATUS register
        // break;
        case iicRegister::RELAY:

            // asyncAction = NOOP;

            switch (data)
            {
            case relayCmd::OFF: // fallthrough all allowed commands for relay
            case relayCmd::ON:
            case relayCmd::TOGGLE:
                relayAction(data);
                break;
            default:
                break;
            } // switch (data)

            // Store in status register is done externally upon real switch
            // Store in EEPROM is done externally upon real switch
            break;
        case iicRegister::LED:
            // Set LED state
            // 0: OFF, 1: ON, 2: TOGGLE, 3: Follow relay state
            switch (data)
            {
            case ledCmd::FOLLOW_RELAY: // fallthrough all allowed commands for LED
            case ledCmd::UNFOLLOW_RELAY:
            case ledCmd::OFF:
            case ledCmd::ON:
            case ledCmd::TOGGLE:
                ledAction(data);
                break;
            default:
                break;
            }
            // Store in status register
            // Store in EEPROM?
            break;
        case iicRegister::BUTTON:
            // Set button state
            // Store in status register
            // Store in EEPROM?
            break;
        case iicRegister::ADDR:
            // Set new IIC address during first initialization
            // Master calls Initial IIC address and updates to new one
            if (!data || data >= 127)
                // Sanity check on new IIC address.
                return;
            // Store in status register
            iicSlaveAddress = data;
            // Store in EEPROM
            EEPROM.begin();
            EEPROM.write(EEPIDX_IICADDR, iicSlaveAddress);
            EEPROM.end();
            break;
        // case iicRegister::VERSION:
        // Ignore writes to VERSION register
        // break;
        default:
            break;
        } // switch (iicRegSelected)

        // Advance to next register in case we got multiple data values.
        // In case of last register roll over to first one again.
        iicRegSelected++;
        if (iicRegSelected >= iicRegSize)
        {
            iicRegSelected = 0;
        }

    } // while (howMany--)

} // iicReceiveEventCb

/**
 * ewlSaveConfig
 * 
 */
void ewlSaveConfig()
{

    dbg(F("elwSaveConfig: Status: ") << statusReg << endl);
    dbg(F("elwSaveConfig: Relay: ") << relayReg << endl);
    dbg(F("elwSaveConfig: LED: ") << ledReg << endl);

    EEPROMwl.update(EWLIDX_STATUS, statusReg);
    EEPROMwl.update(EWLIDX_RELAY, relayReg);
    EEPROMwl.update(EWLIDX_LED, ledReg);

} // ewlSaveConfig

/**
 * ewlLoadConfig
 * 
 */
void ewlLoadConfig()
{

    statusReg = EEPROMwl.read(EWLIDX_STATUS);
    relayReg = EEPROMwl.read(EWLIDX_RELAY);
    ledReg = EEPROMwl.read(EWLIDX_LED);

} // ewlLoadConfig

void readConfigFromEeprom(void)
{
    dbg(F("readConfigFromEeprom") << endl);

    dumpEEPROM();

    // Read stable part of config w/o wear leveling
    EEPROM.begin();
    // EEPROM.get(0, &iicSlaveAddress);
    iicSlaveAddress = EEPROM.read(EEPIDX_IICADDR);
    EEPROM.end();

    // Read wear level protected part of config
    ewlLoadConfig();

} // readConfigFromEeprom

void setup()
{
#ifdef DEBUG
    txOnlySerial.begin(txOnlySerialBaudRate);
#endif

    dbg(endl
        << F("RELAY Module --- debug on") << endl);

    persistIicRegs = false;

    // Read config from EEPROM
    // iicSlaveAddress = 125;
    // Relay Mode
    // LED Mode
    // Button Mode

    statusReg = 0;
    relayReg = 0;
    ledReg = 0;
    buttonReg = 0;

    EEPROMwl.begin(EWL_LAYOUT_VERSION, EWLIDX_COUNT, EWL_USE_BYTES);
    readConfigFromEeprom();

    pinMode(PIN_RL1, OUTPUT);
    digitalWrite(PIN_RL1, RELAY_COIL_OFF);

    pinMode(PIN_RL2, OUTPUT);
    digitalWrite(PIN_RL2, RELAY_COIL_OFF);

#ifndef DEBUG
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LED_OFF);
#endif

    // pinMode(PIN_BUTTON, INPUT);

    iicSlaveAddress = 125;

    // Initialize IIC
    TinyWireS.begin(iicSlaveAddress);

    TinyWireS.onRequest(iicRequestEventCb);
    TinyWireS.onReceive(iicReceiveEventCb);

} // setup

void loop()
{

    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    // sleep_enable();
    // sleep_mode();
    // sleep_disable();

    // if (asyncAction != NOOP)
    // {
    //     relayAction(asyncAction);
    //     asyncAction = NOOP;
    // }

    if (persistIicRegs)
    {
        ewlSaveConfig();
        persistIicRegs = false;

        dumpEEPROM();
    }

    TinyWireS_stop_check();

} // loop

/**
 * dumpEEPROM
 * 
 * Dumps the contents of 
 * Function gets optimized away when DEBUG is not set.
 * 
 */
void dumpEEPROM(void)
{
#ifdef DEBUG

    unsigned int idx = 0;
    uint8_t col = 0;
    const uint8_t MAX_COL = 16;
    const unsigned int EEPROM_SIZE = 512;

    EEPROM.begin();

    dbg(_WIDTHZ(_HEX(idx), 3) << F(" : "));

    for (idx = 0; idx < EEPROM_SIZE; idx++)
    {
        uint8_t data = EEPROM.read(idx);
        dbg(_WIDTHZ(_HEX(data), 2));
        col++;
        switch (col)
        {
        case 8:
            dbg(F(" : "));
            break;
        case 16:
            dbg(endl);
            col = 0;
            if (idx < EEPROM_SIZE - MAX_COL)
            {
                dbg(_WIDTHZ(_HEX(idx), 3) << F(" : "));
            }
            break;
        default:
            dbg(" ");
            break;
        }
    }

    EEPROM.end();

#endif

} // dumpEEPROM