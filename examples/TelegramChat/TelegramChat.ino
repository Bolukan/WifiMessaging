#include <Arduino.h>
#include <wifimessaging.h>

void callback(char *topic, byte *payload, unsigned int length);

// WiFi and MQTT
WifiMessaging myWM(WifiMessaging::connectionService::ServiceWifi |
                       WifiMessaging::connectionService::ServiceMQTT |
                       WifiMessaging::connectionService::ServiceNTP |
                       WifiMessaging::connectionService::ServiceSecure |
                       WifiMessaging::connectionService::ServiceTelegram,
                   callback);

bool sentMessage = false;
const char *MQTT_TOPIC = "test/test";
const char *MQTT_PAYLOAD = "SIGNAL";

void callback(char *topic, byte *payload, unsigned int length)
{
  // DEBUG: Show payload
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // */
}

void setup()
{
  if (!Serial) Serial.begin(115200);
  Serial.println("\nWifi Messaging test.");

  myWM.connectToWiFi();
}

void loop()
{
  // Start MQTT after WiFi, Secure after NTP, Telegram after Secure
  myWM.loop();

  if (
      (myWM.StatusTelegram == myWM.ConnectionActive) &&
      (myWM.StatusMQTT == myWM.ConnectionActive) &&
      (!sentMessage))
  {
    myWM.mqttClient.publish(MQTT_TOPIC, MQTT_PAYLOAD);
    myWM.sendMessage(String(MQTT_PAYLOAD), String(""));
    sentMessage = true;
    Serial.println("Message sent. Be proud! (End of messages)");
  }
}