#include <Arduino.h>
#include <Streaming.h>

#include "iotwebconf_.h"
#include "mqtt.h"
#include "ntp.h"
#include "ota.h"
#include "relay.h"

bool needReset = false;

/**
 * setup
 * 
 */
void setup()
{

    Serial.begin(2000000);
    Serial << endl
           << appName << F(" starting up...\n");

    setupIotWebConf();
    ota::setup();
    ntp::setup();
    mqtt::setup();
    relay::setup();

    Serial << F("Heap: ") << system_get_free_heap_size() << endl;
    system_print_meminfo();
    Serial << endl;

    Serial << F("Global setup done.") << endl;
}   // setup

/**
 * loop
 * 
 */
void loop()
{

    loopIotWebConf();

    ota::loop();
    mqtt::loop();
    ntp::loop();

    relay::loop();

    if (needReset)
    {
        Serial << F("Reboot requested\n");
        // iotWebConf.delay(1000);
        delay(1000);
        ESP.restart();
    }

}   // loop