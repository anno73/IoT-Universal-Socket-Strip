# Relay Board Software

ATTiny85

Platformio

VS Code

[I2C TinyWireSIO](https://github.com/rambo/TinyWire)

[EEPROMWearLevel](https://github.com/PRosenb/EEPROMWearLevel)

[SendOnlySoftwareSerial](https://github.com/nickgammon/SendOnlySoftwareSerial)

[Streaming](https://github.com/janelia-arduino/Streaming)

Does each device need to remember configuration or should it get updated by host on power up?

# Supporting Stuff

## Digistump Digispark USB Dev

### Platformio tweaks

I have encountered some issues with the Digistump avr package. There is missing EEPROM.h library. To resolve this I copied the original library from the arduino avr package to .platformio/packages/framework-avr-digistump/libraries package.
There is also a lot of unnecessary noise during compilation because of moved header file locations, redefinition of #define BIN and other stuff. 
In the end I overwrote core and variant to tiny and tinyx5 as it is used by the generic ATTiny85 definitions. Keeping the rest as it is, allowed me to program regularly via USB.

### Windows Driver Installation

https://github.com/micronucleus/micronucleus/tree/master/windows_driver_installer

Use [zadig](https://zadig.akeo.ie/).

### Initial Flashing

[Connecting and Programming Your Digispark](http://digistump.com/wiki/digispark/tutorials/connecting)

### Flash digiStump Digispark USB dev board

[Digispark USB Development Board](http://digistump.com/products/1) Webshop listing

[Schematic](https://s3.amazonaws.com/digistump-resources/files/97a1bb28_DigisparkSchematic.pdf)

[Micronucleus](https://github.com/micronucleus/micronucleus) Bootloader

## C/C++

[Enumeration declaration](https://en.cppreference.com/w/cpp/language/enum)

## Arduino related

[Arduino Tutorial: Using millis[] Instead of delay[]](https://www.norwegiancreations.com/2017/09/arduino-tutorial-using-millis-instead-of-delay/)

[Arduino Tutorial: Avoiding the Overflow Issue When Using millis[] and micros[]](https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/)


## I2C Resources

[I2C [Inter-Integrated Circuit] Bus Technical Overview and Frequently Asked Questions](https://www.esacademy.com/en/library/technical-articles-and-documents/miscellaneous/i2c-bus.html)

[Nick Gammon - I2C - Two-Wire Peripheral Interface - for Arduino](http://www.gammon.com.au/forum/?id=10896)

https://www.i2c-bus.org/




## Level shifter

?? Use I2C against 5V rail? ESP8266 is 5V tolerant.

As it seems, a 5V powered ATTiny85 does not recognize 3.3V levels on I2C. So a level shifter needs to be used to convert to 5V levels as used by the ATTiny.

[sparkfun - Bi-Directional Logic Level Converter Hookup Guide](https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide/all)


## Fuse Calc and Fuse Reset

[Engbedded Atmel AVR® Fuse Calculator](https://www.engbedded.com/fusecalc/)

https://www.elektronik-labor.de/Arduino/Fuses.html

TinyCalibrator from below

https://arduinodiy.wordpress.com/2015/05/16/high-voltage-programmingunbricking-for-attiny/

[Simple and Cheap Fuse Doctor for Attiny](https://www.instructables.com/Simple-and-cheap-Fuse-Doctor-for-Attiny/)


## Flash ATTiny85

### High Voltage Programming

[Nick Gammon - High-voltage programming for AVR chips](http://www.gammon.com.au/forum/?id=12898)

### Flash ATTiny85

Arduino as a programmer




### Boot loader

[ATtiny85 I2C Bootloader](https://github.com/casanovg/timonel) to flash via ESP8266.
https://www.youtube.com/watch?v=-7GOMToGvzI

[Micronucleus](https://github.com/micronucleus/micronucleus) V-USB based.

## Debugging

### Serial TX only

https://github.com/nickgammon/SendOnlySoftwareSerial

### debugWire

https://sites.google.com/site/wayneholder/debugwire
https://www.youtube.com/watch?v=kI_Z78a_0y0

## Calibrate RC Oscillator

* http://ww1.microchip.com/downloads/en/appnotes/atmel-2555-internal-rc-oscillator-calibration-for-tinyavr-and-megaavr-devices_applicationnote_avr053.pdf
* https://hackaday.com/2020/12/30/improve-attiny-timing-accuracy-with-this-clock-calibrator/
* https://hackaday.io/project/176542-tiny-calibrator
* https://github.com/wagiminator/ATtiny84-TinyCalibrator


