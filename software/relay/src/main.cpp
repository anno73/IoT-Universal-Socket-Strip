#include <Arduino.h>

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

#include "shared.h"

//#include <SendOnlySoftwareSerial.h>       // https://github.com/nickgammon/SendOnlySoftwareSerial

//#include <EEPROM.h>
//#include <EEPROMWearLevel.h>    // https://github.com/PRosenb/EEPROMWearLevel

#include <TinyWireS.h> // https://github.com/nickcengel/TinyWireSio
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE (16)
#endif

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
volatile uint8_t statusReg = 0;
volatile uint8_t relayReg = 0;
volatile uint8_t ledReg = 0;
volatile uint8_t buttonReg = 0;
// volatile uint8_t addrReg = 0;    // does not need to be a physical register. Is obtained from EEPROM.
// volatile uint8_t versionReg = 0; // does not need to be a physical register. Is a constant in the code.

// Register number as seen on IIC bus. May also be used as index to register union struct array -> use enum iicRegister::...
// const uint8_t IIC_REG_STATUS_IDX = 0;
// const uint8_t IIC_REG_RELAY_IDX = 1;
// const uint8_t IIC_REG_LED_IDX = 2;
// const uint8_t IIC_REG_BUTTON_IDX = 3;
// const uint8_t IIC_REG_IIC_ADDR_IDX = 4;
// const uint8_t IIC_REG_VERSION_IDX = 5;

volatile uint8_t iicRegPosition;
// const uint8_t iicRegSize = sizeof(iicRegs);
const uint8_t iicRegSize = 6;
const uint8_t iicRegLastIdx = 5;

enum Action
{
  NOOP,
  RELAY_OFF,
  RELAY_ON,
  RELAY_TOGGLE
};

volatile Action asyncAction = NOOP; // does is need to be volatile? Does it hurt if it is?

/**
 * runAsyncAction
 * 
 * Execute operations that take longer than allowed in the IIC callback routine.
 * This is mainly the relay switching, as it requires some delay on the coil to properly switch.
 * 
 */
void runAsyncAction(Action action)
{
  switch (action)
  {
  case RELAY_OFF:
    // iicRegs[IIC_REG_STATUS] &= ~1;
    statusReg &= ~1;
    relayReg &= ~1;

    digitalWrite(PIN_RL1, RELAY_COIL_ON);
    tws_delay(RELAY_SWITCH_PULSE_DURATION_MS);
    digitalWrite(PIN_RL1, RELAY_COIL_OFF);

    digitalWrite(PIN_LED, LED_OFF);
    break;
  case RELAY_ON:
    // iicRegs[IIC_REG_STATUS] |= 1;
    statusReg |= 1;
    relayReg |= 1;

    digitalWrite(PIN_RL2, RELAY_COIL_ON);
    tws_delay(RELAY_SWITCH_PULSE_DURATION_MS);
    digitalWrite(PIN_RL2, RELAY_COIL_OFF);

    digitalWrite(PIN_LED, LED_ON);
    break;
  case RELAY_TOGGLE:
    if (relayReg & 1)
    {
      runAsyncAction(RELAY_OFF);
    }
    else
    {
      runAsyncAction(RELAY_ON);
    }
    break;
  default:
    break;
  }


  return;
} // runAsyncAction

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

  // digitalWrite(PIN_LED, !digitalRead(PIN_LED));

  switch (iicRegPosition)
  {
  case iicRegister::STATUS:
    TinyWireS.send(statusReg);
    break;
  case iicRegister::RELAY:
    TinyWireS.send(0xF1);
    break;
  case iicRegister::LED:
    TinyWireS.send(0xF2);
    break;
  case iicRegister::BUTTON:
    TinyWireS.send(0xF3);
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
  iicRegPosition++;

  // Treat registers as ring buffer. Check for possible wrap around and handle it.
  if (iicRegPosition >= iicRegLastIdx)
  {
    iicRegPosition = 0;
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

  // digitalWrite(PIN_LED, !digitalRead(PIN_LED));

  if (howMany < 1)
    // Sanity check
    return;

  if (howMany > TWI_RX_BUFFER_SIZE)
    // Sanity check - should no occur really
    return;

  iicRegPosition = TinyWireS.receive();
  howMany--;

  if (!howMany)
    // This write was only to select the register for next read operation (iicRequestEvent)
    return;

  // Seems we got a valid write request with at least another byte of data
  // iicRegPosition indicates what register to start with as subsequent received bytes advance to the next register

  while (howMany--)
  {
    uint8_t data = TinyWireS.receive(); 

    switch (iicRegPosition)
    {
    // case iicRegister::STATUS:
    // Ignore writes to STATUS register
    // break;
    case iicRegister::RELAY:

      asyncAction = NOOP;

      // todo: check if relay is frozen or not

      switch (data)
      {
      case relayCmd::OFF:
        // asyncAction = RELAY_OFF;
        runAsyncAction(RELAY_OFF);
        break;
      case relayCmd::ON:
        // asyncAction = RELAY_ON;
        runAsyncAction(RELAY_ON);
        break;
      case relayCmd::TOGGLE:
        // asyncAction = RELAY_TOGGLE;
        runAsyncAction(RELAY_TOGGLE);
        break;
      default:
        // asyncAction = NOOP;
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
      case ledCmd::FOLLOW_RELAY:
        // todo
        break;
      case ledCmd::ALWAYS_OFF:
        digitalWrite(PIN_LED, LED_OFF);
        break;
      case ledCmd::ALWAYS_ON:
        digitalWrite(PIN_LED, LED_ON);
        break;
      case ledCmd::TOGGLE:
        digitalWrite(PIN_LED, digitalRead(PIN_LED) == LED_OFF ? LED_ON : LED_OFF);
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
      // Store in status register
      // Store in EEPROM
      break;
    // case iicRegister::VERSION:
      // Ignore writes to VERSION register
      // break;
    default:
      break;
    } // switch (iicRegPosition)

    // Advance to next register in case we got multiple data values.
    // In case of last register roll over to first one again.
    iicRegPosition++;
    if (iicRegPosition >= iicRegSize)
    {
      iicRegPosition = 0;
    }

  } // while (howMany--)

} // iicReceiveEventCb

void setup()
{

  // Read config from EEPROM
  // iicSlaveAddress = 125;
  // Relay Mode
  // LED Mode
  // Button Mode

  statusReg = 0;
  relayReg = 0;
  ledReg = 0;
  buttonReg = 0;

  pinMode(PIN_RL1, OUTPUT);
  digitalWrite(PIN_RL1, RELAY_COIL_OFF);

  pinMode(PIN_RL2, OUTPUT);
  digitalWrite(PIN_RL2, RELAY_COIL_OFF);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF);
  // pinMode(PIN_BUTTON, INPUT);

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

  if (asyncAction != NOOP)
  {
    runAsyncAction(asyncAction);
    asyncAction = NOOP;
  }

  TinyWireS_stop_check();

} // loop