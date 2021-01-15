#include <Arduino.h>

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

//#include <SendOnlySoftwareSerial.h>       // https://github.com/nickgammon/SendOnlySoftwareSerial

//#include <EEPROM.h>
//#include <EEPROMWearLevel.h>    // https://github.com/PRosenb/EEPROMWearLevel

#include <TinyWireS.h>
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
 *   6      PB6     1
 *   7      PB7    2/A1     IIC SCL
 *   8              -       VCC
 */

const uint8_t PIN_SCL = 0;
const uint8_t PIN_SDA = 2;
const uint8_t PIN_LED = 4;
const uint8_t PIN_RL1 = 3;
const uint8_t PIN_RL2 = 1;
const uint8_t PIN_BUTTON = 5;

const uint32_t IIC_BITRATE = 100000;
volatile uint8_t iicSlaveAddress;

const uint8_t RELAY_SWITCH_PULSE_DURATION_MS = 15;
const uint8_t RELAY_COIL_OFF = LOW;
const uint8_t RELAY_COIL_ON = HIGH;

const uint8_t LED_ON = LOW;
const uint8_t LED_OFF = HIGH;

/*
  Operations

  Set relay state
  Get relay state
  Set LED operation
    0: Follow Relay
    1: 
    2: Always on
      0: off
      1: on
    4: Blink
      3..0: rate
  Get LED status
  Set button operation
    0: Toggle relay
    1: off
  Get button status
  Set IIC Address
  Get IIC Address
  Get Software Version
*/

volatile uint8_t iicRegs[] = {
    0x00, // 0: Status Register
    0x00, // 1: Set relay state
    0x00, // 2: IIC Address Register
    0x00  // 3: Software version
};

const uint8_t IIC_REG_STATUS = 0;
const uint8_t IIC_REG_RELAY = 1;
const uint8_t IIC_REG_LED = 2;
const uint8_t IIC_REG_BUTTON = 3;
const uint8_t IIC_REG_IIC_ADDR = 4;
const uint8_t IIC_REG_VERSION = 5;

const uint8_t RELAY_STATE = 1;
const uint8_t LED_OPERATION = 2;
const uint8_t LED_STATE = 4;
const uint8_t BUTTON_OPERATION = 8;
const uint8_t BUTTON_STATE = 16;

volatile uint8_t iicRegStatus;

volatile uint8_t iicRegPosition;
const uint8_t iicRegSize = sizeof(iicRegs);
const uint8_t iicRegLastIdx = 5;

enum Action
{
  NOOP,
  RELAY_OFF,
  RELAY_ON,
  RELAY_TOGGLE
};

volatile Action asyncAction = NOOP;

/**
 * runAsyncAction
 * 
 * Execute operations that take longer than allowed in the IIC callback routine.
 * This is mainly the relay switching as it requires some delay on the coil to properly switch.
 */
void runAsyncAction(Action action)
{
  switch (action)
  {
  case RELAY_OFF:
    iicRegs[IIC_REG_STATUS] &= ~1;
    digitalWrite(PIN_RL1, HIGH);
    tws_delay(RELAY_SWITCH_PULSE_DURATION_MS);
    digitalWrite(PIN_RL1, LOW);
    digitalWrite(PIN_LED, LED_OFF);
    break;
  case RELAY_ON:
    iicRegs[IIC_REG_STATUS] |= 1;
    digitalWrite(PIN_RL2, HIGH);
    tws_delay(RELAY_SWITCH_PULSE_DURATION_MS);
    digitalWrite(PIN_RL2, LOW);
    digitalWrite(PIN_LED, LED_ON);
    break;
  case RELAY_TOGGLE:
    if (IIC_REG_STATUS & 1)
      runAsyncAction(RELAY_OFF);
    else
      runAsyncAction(RELAY_ON);

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
  uint8_t tmp;

  switch (iicRegPosition)
  {
  case IIC_REG_STATUS:
    TinyWireS.send(iicRegStatus);
    break;
  case IIC_REG_RELAY:
    TinyWireS.send(0xFF);
    break;
  case IIC_REG_LED:
    TinyWireS.send(0xFF);
    break;
  case IIC_REG_BUTTON:
    TinyWireS.send(0xFF);
    break;
  case IIC_REG_IIC_ADDR:
    TinyWireS.send(0xFF);
    break;
  case IIC_REG_VERSION:
    TinyWireS.send(0xFF);
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

  // We got a valid write request with at least another byte of data
  // iicRegPosition indicates what to do

  while (howMany--)
  {
    uint8_t data = TinyWireS.receive(); // just consume for now

    switch (iicRegPosition)
    {
    case IIC_REG_RELAY:
      // Set relay state
      // 0: OFF, 1: ON, 2: TOGGLE
      switch (data)
      {
      case 0:
        asyncAction = RELAY_ON;
        break;
      case 1:
        asyncAction = RELAY_OFF;
        break;
      case 2:
        asyncAction = RELAY_TOGGLE;
        break;
      default:
        asyncAction = NOOP;
        break;
      }
      // Store in status register
      // Store in EEPROM?
      break;
    case IIC_REG_LED:
      // Set LED state
      // Store in status register
      // Store in EEPROM?
      break;
    case IIC_REG_BUTTON:
      // Set button state
      // Store in status register
      // Store in EEPROM?
      break;
    case IIC_REG_IIC_ADDR:
      // Set new IIC address during first initialization
      // Master calls Initial IIC address and updates to new one
      // Store in status register
      // Store in EEPROM
      break;
    case IIC_REG_VERSION:
      break;
    default:
      break;
    }

    // i2c_regs[reg_position] = TinyWireS.receive();
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
  iicSlaveAddress = 10;
  // Relay Mode
  // LED Mode
  // Button Mode

  pinMode(PIN_RL1, OUTPUT);
  pinMode(PIN_RL2, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);



  // Initialize IIC
  TinyWireS.begin(iicSlaveAddress);

  TinyWireS.onRequest(iicRequestEventCb);
  TinyWireS.onReceive(iicReceiveEventCb);


} // setup

void loop()
{

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  sleep_disable();

  if (asyncAction != NOOP)
    runAsyncAction(asyncAction);

  TinyWireS_stop_check();

} // loop