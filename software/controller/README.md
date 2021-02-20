# Controller Board Software

ESP8266

[PlatformIO](https://platformio.org)

[VS Code](https://code.visualstudio.com/)

[IotWebConf](https://github.com/prampec/IotWebConf)

[MQTT](https://github.com/256dpi/arduino-mqtt)

[ArduinoJSON](https://arduinojson.org/)

[I2C - Wire.h](https://github.com/esp8266/Arduino/tree/master/libraries/Wire)

NTP - ESP8266 native

PolledTimeout - ESP8266 native ([Example](https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/BlinkPolledTimeout/BlinkPolledTimeout.ino))

AJAX?

# Supporting Stuff

## Arduino related

## Arduino ESP8266 Core related

[ESP8266 Blink with polledTimeout by Daniel Salazar](https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/BlinkPolledTimeout/BlinkPolledTimeout.ino) instead of millis() workaround.

## Exception Stack Trace Decoder

https://github.com/me-no-dev/EspExceptionDecoder

## I2C 

### Wire library

I encountered persisting problems with reading data from relay board. What solved it in the end was to add Wire.setClockStretchLimit() after Wire.begin():
```
Wire.begin(PIN_SDA, PIN_SCL, 1);  // Init IIC with master address 1
Wire.setClock(100000);            // 100kHz clock speed
Wire.setClockStretchLimit(40000); // 40ms
```



### I2C Level shifter

?? Use I2C against 5V rail? ESP8266 is 5V tolerant.

As it seems, a 5V powered ATTiny85 does not recognize 3.3V levels on I2C. So a level shifter needs to be used to convert to 5V levels as used by the ATTiny.

[sparkfun - Bi-Directional Logic Level Converter Hookup Guide](https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide/all)


