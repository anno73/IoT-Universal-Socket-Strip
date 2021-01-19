/**
 * 
 * Shared datatypes between controller and relay
 * 
 * 
 */
#pragma once

#include <Arduino.h>

/**
 * The fight on a more verbose enum as there is a compile time conflict with enumerator STATUS.
 * https://en.cppreference.com/w/cpp/language/enum
 * 
 * Used to share register index values in a transparent and maintainable way between controller and relay.
 * Access like iicRegister::STATUS
 * 
 */

namespace iicRegister
{
    enum : uint8_t
    {
        STATUS = 0,
        RELAY = 1,
        LED = 2,
        BUTTON = 3,
        ADDR = 4,
        VERSION = 5,
    };
}

namespace relayCmd {
    enum : uint8_t {
        OFF = 0,
        ON = 1,
        TOGGLE = 2,
        FREEZE = 3,
        UNFREEZE = 4,
    };
}

namespace ledCmd
{
    enum : uint8_t
    {
        FOLLOW_RELAY = 0,
        ALWAYS_OFF = 1,
        ALWAYS_ON = 2,
        TOGGLE = 3,
        FREEZE = 4,
        UNFREEZE = 5,
        BLINK0 = 0x03,
        BLINK1 = 0x13,
        BLINK2 = 0x23,
        BLINK3 = 0x33,
    };
}

namespace buttonCmd {
    enum : uint8_t {
        ENABLE = 0,
        DISABLE = 1,
    };
}