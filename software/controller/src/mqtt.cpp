// https://github.com/256dpi/arduino-mqtt
#include <MQTT.h>
#include <MQTTClient.h>

#include <Arduino.h>
#include <Streaming.h>
#include <TimeLib.h>

#include <ESP8266WiFi.h>

// https://arduinojson.org/v6/doc/
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

//#include "mqtt.h"
#include "global.h"
#include "ntp.h"
#include "iotwebconf_.h"
#include "relay.h"

/*

  Missing actions: 


  Implemented
    'mqttTimeTopic'

    /cmd
      /reboot

    /set
      /on
      /off
      /toggle

    /get

*/

namespace mqtt {

const uint16_t MQTT_BUFFER_SIZE = 256;

char mqttServer[MQTT_SERVER_STR_LEN] = "127.0.0.1";

char mqttPort[MQTT_PORT_STR_LEN] = "1883";
uint16_t mqttPortInt = 0;

char mqttTopicPraefix[MQTT_TOPIC_PRAEFIX_STR_LEN] = "";
uint16_t mqttTopicPraefixLength = 0;

char mqttConnectRetryDelay[MQTT_CONNECT_RETRY_DELAY_STR_LEN] = "5000";
uint16_t mqttConnectRetryDelayInt = 0;

char mqttHeartbeatInterval[MQTT_HEARTBEAT_INTERVALL_STR_LEN] = "60000"; // set to 0 to turn off heartbeat
unsigned long mqttHeartbeatIntervalInt;

bool mqttDisabled = true;
char mqttTimeTopic[MQTT_TIME_TOPIC_STR_LEN] = "";
bool mqttTimeTopicSet = false;

// wifiConnected callback indicates that MQTT can now connect to the broker
bool needConnect = false;

//MQTTClient mqttClient(MQTT_BUFFER_SIZE);
MQTTClient mqttClient(256);

void mqttSendHeartbeat();
void mqttMessageReceived(String &, String &);

//
//
//
void setup()
{

  Serial << F("Setup MQTT") << endl;

  mqttClient.begin(mqttServer, mqttPortInt, wifiClient);
  //  mqttClient.setTimeout(100);
  mqttClient.setWill("lastWill", "disconnected", true, 0);
  mqttClient.onMessage(mqttMessageReceived);

  // Subscribe to topics after successful connection

  if ((mqttPort == 0) || (mqttServer[0] == '\0') || (mqttTopicPraefix[0] == '\0'))
  {
    Serial << F("MQTT disabled due to missing or wrong parameters given\n");
    mqttDisabled = true;
  }
  else
  {
    mqttDisabled = false;
  }

} // setupMqttClient

//
//
//
bool connectMqtt()
{
  static unsigned long lastConnectionAttempt = 0; // persist across calls
  unsigned long now = millis();

  if (mqttDisabled)
  {
    return false;
  }

  if (mqttConnectRetryDelayInt > now - lastConnectionAttempt)
  {
    // We are not due for a connection attempt
    // Tell caller we did not connect
    return false;
  }

  Serial << F("MQTT - Trying to connect\n");

  if (!mqttClient.connect(iotWebConf.getThingName()))
  {
    lastConnectionAttempt = now;
    Serial << F("MQTT Connection to ") << mqttServer << ':' << mqttPortInt << F(" failed: ") << mqttClient.lastError() << ':' << mqttClient.returnCode() << F(". Will try again in ") << mqttConnectRetryDelayInt << F("ms") << endl;
    return false;
  }

  Serial << F("MQTT Connected\n");

  // Subscribe to required topics
  String s;
  s = mqttTopicPraefix;
  s += "/#";
  mqttClient.subscribe(s);
  Serial << F("MQTT subscribe to ") << s << endl;

  if (mqttTimeTopic[0] != 0)
  {
    s = mqttTimeTopic;
    mqttClient.subscribe(s);
    Serial << F("MQTT subscribe to ") << s << endl;
  }
  return true;
} // connectMqtt

//
//
//
void loop()
{

  if (!mqttClient.loop())
  {
    //      Serial << F("MQTT client.loop error: ") << mqttClient.lastError() << ':' << mqttClient.returnCode() << endl;
  }

  if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected()))
  {
    needConnect = true;
  }

  if (needConnect)
  {
    if (connectMqtt())
    {
      needConnect = false;
    }
  }

  mqttSendHeartbeat();

} // loopMqtt

void mqttSendHeartbeat()
{

  static unsigned long mqttNextHeartbeat = 0;

  unsigned long _now = millis();

  // Send MQTT heartbeat every once in a while
  // set mqttHeartbeatIntervalInt=0 to turn off
  if (mqttClient.connected() && mqttHeartbeatIntervalInt && _now >= mqttNextHeartbeat)
  {
    String topic = mqttTopicPraefix;
    topic += "/info/heartbeat";

    const int jsonCapacity = JSON_OBJECT_SIZE(10);
    StaticJsonDocument<jsonCapacity> doc;

    //    doc["time"] = ntpClient->getFormattedTime();
    doc["time"] = ntp::dateTimeStr(time(nullptr), "%Y-%m-%d %H:%M:%S");
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["SSID"] = WiFi.SSID();
    doc["RSSI"] = WiFi.RSSI();
    doc["MAC"] = WiFi.macAddress();
    doc["IP"] = WiFi.localIP().toString();

    String json;
    serializeJsonPretty(doc, json);
    Serial << F("MQTT send heartbeat [") << topic << F("] with ") << json.length() << F(" bytes:\n") << json << endl;

    json = ""; // serializeJson* APPENDS to target String object!
    serializeJson(doc, json);
    bool rc = mqttClient.publish(topic, json);

    if (!rc)
    {
      Serial << F("MQTT publish error: ") << mqttClient.lastError() << ':' << mqttClient.returnCode() << endl;
    }
    mqttNextHeartbeat = _now + mqttHeartbeatIntervalInt;
  }

} // mqttSendHeartbeat

typedef struct _pack
{
  char year[5];
  char month[3];
  char day[3];
  char hour[3];
  char min[3];
  char sec[3];
} timeStrPack;

typedef union _timeStr
{
  char buf[20];
  timeStrPack pack;
} timeStr;

//
//  Process received MQTT messages
//
void mqttMessageReceived(String &topic, String &data)
{
  Serial << F("MQTT message received on '") << topic << F("' with ") << data.length() << F(" bytes: '") << data << F("'\n");

  //  if (strncmp(topic.c_str(), mqttTimeTopic, sizeof(mqttTimeTopic)) == 0 ) {
  if (mqttTimeTopicSet && topic.startsWith(mqttTimeTopic))
  {
    Serial << F("Received Time update: ") << data << endl;
    timeStr buf;

    // "2017-01-25T21:35:18"
    //      4  3  3  3  3  3
    strncpy(buf.buf, data.c_str(), sizeof(buf.buf));
    buf.buf[4] = 0;
    buf.buf[7] = 0;
    buf.buf[10] = 0;
    buf.buf[13] = 0;
    buf.buf[16] = 0;
    buf.buf[19] = 0;
    setTime(atoi(buf.pack.hour),
            atoi(buf.pack.min),
            atoi(buf.pack.sec),
            atoi(buf.pack.day),
            atoi(buf.pack.month),
            atoi(buf.pack.year));

    //    timeValid = true;

    return;
  }

  topic.remove(0, mqttTopicPraefixLength);

  Serial << F("MQTT action: ") << topic << endl;

  if (topic.startsWith("/info"))
  {
    // Sent by us. Gracefully ignore
    Serial << F("MQTT ignore action ") << topic << endl;
    return;
  }

  if (topic.startsWith("/cmd/"))
  {

    topic.remove(0, 5);

    if (topic.startsWith("reboot"))
    {
      needReset = true;
      return;
    }

    Serial << F("MQTT unknown cmd '") << topic << F("'\n");
    return;
  } // cmd

  if (topic.startsWith("/set/"))
  {

    topic.remove(0, 5);

    if (topic.startsWith("on"))
    {
      uint8_t v = strtoul(data.c_str(), NULL, 10);

      Serial << F("MQTT turn socket") << v << F("on") << endl;
      relay::switchRelay(v, 1);
      return;
    }

    if (topic.startsWith("off"))
    {
      uint8_t v = strtoul(data.c_str(), NULL, 10);

      Serial << F("MQTT turn socket ") << v << F("off") << endl;
      relay::switchRelay(v, 0);
      return;
    }

    if (topic.startsWith("toggle"))
    {
      uint8_t v = strtoul(data.c_str(), NULL, 10);

      Serial << F("MQTT toggle socket ") << v << endl;
      relay::switchRelay(v, 2);
      return;
    }

    // if ( topic.startsWith(F("mode")) ) {
    //   // -2: prev, -1: next, >=0: abs
    //   int8_t v = strtoul(data.c_str(), NULL, 10);

    //   Serial << F("MQTT set new mode: ") << v << endl;
    //   // setModeRGB(v);
    //   return;
    // }

    // if ( topic.startsWith(F("intensity")) ) {
    //   // -n, 0..255, +n

    //   if ( data.length() == 0 )
    //     return;

    //   // Is number realtive (start with +/-) or absolute (start with digit)
    //   bool absolute = isDigit( data[0] );

    //   if (absolute) {
    //     uint8_t v = strtoul(data.c_str(), NULL, 10);
    //     Serial << F("MQTT set new intensity: ") << v << endl;
    //     // setAbsoluteBrightnessRGB(v);
    //     return;
    //   } else {
    //     int8_t v = strtoul(data.c_str(), NULL, 10);
    //     Serial << F("MQTT set new relative intensity: ") << v << endl;
    //     // setRelativeBrightnessRGB(v);
    //     return;
    //   }

    //   return;
    // }

    Serial << F("MQTT unknown set '") << topic << "'\n";
    return;
  } // set

  {
    Serial << F("MQTT unknown topic '") << topic << "'\n";
    return;
  }

  // parse topic and react
  return;
} // mqttMessageReceived

} // namespace mqtt