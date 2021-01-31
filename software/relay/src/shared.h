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
        RELAY,
        LED,
        BUTTON,
        ADDR,
        VERSION,
        // --- Meta values
        REGCOUNT, // Not really a register but an indicator on how many registers are defined; gets previous value + 1
    };
}

namespace relayCmd
{
    enum : uint8_t
    {
        OFF = 0,
        ON,
        TOGGLE,
        FREEZE,
        UNFREEZE,
    };
}

namespace ledCmd
{
    enum : uint8_t
    {
        FOLLOW_RELAY = 0x00,
        UNFOLLOW_RELAY,
        OFF,
        ON,
        TOGGLE,
        FREEZE,
        UNFREEZE,
        BLINK0 = 0x08, // Reserve for possible later implementation. Align it for easier use later on
        BLINK1,
        BLINK2,
        BLINK3,
        // Internal meta states. Only to be used in relay module.
        // Maybe not the correct way to do it.
        RELAY_OFF = 0xF0,
        RELAY_ON,
    };
}

namespace buttonCmd // Button not implemented
{
    enum : uint8_t
    {
        ENABLE = 0,
        DISABLE,
    };
}