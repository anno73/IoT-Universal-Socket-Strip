# Relay Board Software 

[ATTiny85](https://www.microchip.com/wwwproducts/en/ATtiny85)

[PlatformIO](https://platformio.org)

[VS Code](https://code.visualstudio.com/)

[I2C TinyWireSIO](https://github.com/rambo/TinyWire)

[EEPROMWearLevel](https://github.com/PRosenb/EEPROMWearLevel)

[SendOnlySoftwareSerial](https://github.com/nickgammon/SendOnlySoftwareSerial)

[Streaming](https://github.com/janelia-arduino/Streaming)

[avr-libc Watchdog](https://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html)

Does each device need to remember configuration or should it get updated by host on power up?

# Supporting Stuff

## Digistump Digispark USB Dev

### Platformio tweaks

#### Switch the platform package

I have encountered some issues with the Digistump avr package. There is missing EEPROM.h library. To resolve this I copied the original library from the arduino avr package to .platformio/packages/framework-avr-digistump/libraries package.
There is also a lot of unnecessary noise during compilation because of moved header file locations, redefinition of #define BIN and other stuff.
In addition EEPROMWearLevel library did not compile. 
This was the final reason for me to try a different core package.
In the end I overwrote core and variant from [Digispark USB](https://docs.platformio.org/en/latest/boards/atmelavr/digispark-tiny.html#board-atmelavr-digispark-tiny) package to tiny and tinyx5 as it is used by the [generic ATTiny85](https://docs.platformio.org/en/latest/boards/atmelavr/attiny85.html#board-atmelavr-attiny85) definitions (see [attiny85.json](https://github.com/platformio/platform-atmelavr/blob/master/boards/attiny85.json)). Keeping the rest as it is, allowed me to program regularly via USB. See [platformio.ini](platformio.ini).

There is a new actively maintained ATTiny85 core available on https://github.com/ArminJo/DigistumpArduino which gives smaller code, has active development, and more stock libraries. I use this now for further development.

#### Include (missing) libs as submodules

As I did not find [SendOnlySoftwareSerial](https://github.com/nickgammon/SendOnlySoftwareSerial) and a current version of [Streaming](https://github.com/janelia-arduino/Streaming) in PlatformIO's library repository, I added these as [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) in the library folder.
See [github blog](https://github.blog/) for more information on [how to work with submodules](https://github.blog/2016-02-01-working-with-submodules/).
Especially on initial cloning you might need to use `git clone --recursive <project url>`. If you already have cloned the repository and only miss the submodules, `git submodule update --init --recursive` should do the trick.

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


## Fuse Calc and Fuse Reset

[Engbedded Atmel AVR® Fuse Calculator](https://www.engbedded.com/fusecalc/)

https://www.elektronik-labor.de/Arduino/Fuses.html

TinyCalibrator from below

https://arduinodiy.wordpress.com/2015/05/16/high-voltage-programmingunbricking-for-attiny/

[Simple and Cheap Fuse Doctor for Attiny](https://www.instructables.com/Simple-and-cheap-Fuse-Doctor-for-Attiny/)


## Flash ATTiny85

### High Voltage Programming

[Nick Gammon - High-voltage programming for AVR chips](http://www.gammon.com.au/forum/?id=12898)

https://github.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser

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

* https://sites.google.com/site/wayneholder/debugwire
* https://www.youtube.com/watch?v=kI_Z78a_0y0

### IIC

[SoftI2CMaster](https://github.com/felias-fogg/SoftI2CMaster) provides [I2CShell](https://github.com/felias-fogg/SoftI2CMaster/tree/master/examples/I2CShell) as an example, which is a nice I2C command sending tool for Arduino.

## Calibrate RC Oscillator

* http://ww1.microchip.com/downloads/en/appnotes/atmel-2555-internal-rc-oscillator-calibration-for-tinyavr-and-megaavr-devices_applicationnote_avr053.pdf
* https://hackaday.com/2020/12/30/improve-attiny-timing-accuracy-with-this-clock-calibrator/
* https://hackaday.io/project/176542-tiny-calibrator
* https://github.com/wagiminator/ATtiny84-TinyCalibrator


