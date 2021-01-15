#include <Arduino.h>
#include <Streaming.h>

#include "iotwebconf_.h"
#include "mqtt.h"
#include "ntp.h"
#include "ota.h"
#include "relay.h"

bool needReset = false;

void setup()
{

  Serial.begin(2000000);
  Serial << endl << appName << F(" starting up...\n");

  setupIotWebConf();
  ota::setup();
  ntp::setup();
  relay::setup();

  Serial << F("Heap: ") << system_get_free_heap_size() << endl;
  system_print_meminfo();
  Serial << endl;

  Serial << F("Global setup done.") << endl;
} // setup

void loop()
{

  loopIotWebConf();

  ota::loop();
  mqtt::loop();
  ntp::loop();

  relay::loop();

} // loop