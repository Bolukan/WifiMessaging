#ifndef WIFIMESSAGING_H
#define WIFIMESSAGING_H

#include <Arduino.h>
#include <Ticker.h>
#include <secrets.h>
#include <certificate.h>
#include <ESP8266WiFi.h>      // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <PubSubClient.h>     // 89              - https://github.com/knolleary/pubsubclient.git
#include <time.h>             // Arduino library - https://github.com/esp8266/Arduino/blob/master/tools/sdk/libc/xtensa-lx106-elf/include/time.h
#include <WiFiClientSecure.h> // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h
#include <UniversalTelegramBot.h>

// **************************************** DEBUG ****************************************

// Uncomment lines below to receive debug messages via Serial
#define DEBUG_WIFIMESSAGING_TO_SERIAL 1

#ifdef DEBUG_WIFIMESSAGING_TO_SERIAL
#define DEBUG_WIFIMESSAGING_BEGIN() \
  if (!Serial)                      \
  Serial.begin(115200)
#define DEBUG_WIFIMESSAGING_PRINT(x) Serial.print(x)
#define DEBUG_WIFIMESSAGING_PRINTLN(x) Serial.println(x)
#define DEBUG_WIFIMESSAGING_PRINTF(...) \
  {                                     \
    Serial.printf("%lu", millis());     \
    Serial.print("mS L");               \
    Serial.print(__LINE__);             \
    Serial.print("/");                  \
    Serial.print(__FUNCTION__);         \
    Serial.print(": ");                 \
    Serial.printf(__VA_ARGS__);         \
  }
#define DEBUG_WIFIMESSAGING_FLUSH() Serial.flush()
#else
#define DEBUG_WIFIMESSAGING_BEGIN()
#define DEBUG_WIFIMESSAGING_PRINT(x)
#define DEBUG_WIFIMESSAGING_PRINTLN(x)
#define DEBUG_WIFIMESSAGING_PRINTF(...)
#define DEBUG_WIFIMESSAGING_FLUSH()
#endif

// **************************************** CLASS ****************************************
// WifiMessaging(ServiceWifi | ServiceNTP | ServiceSecure | ServiceMQTT | ServiceTelegram);
// WifiMessaging.InitialiseMQTT(callback);
// WifiMessaging.connectToWiFi();

/**
 * WiFi and messaging class (h).
 */
class WifiMessaging
{
public:
  enum connectionStatus
  {
    ConnectionActive = 0,
    ConnectionActiveToInactive = 1,
    ConnectionInactiveToActive = 2,
    ConnectionInactive = 3
  };

  enum connectionService : uint16_t
  {
    ServiceWifi = 1,
    ServiceNTP = 2,
    ServiceSecure = 4,
    ServiceMQTT = 8,
    ServiceTelegram = 16
  };

  connectionStatus StatusWiFi = ConnectionInactive;
  connectionStatus StatusNTP = ConnectionInactive;
  connectionStatus StatusMQTT = ConnectionInactive;
  connectionStatus StatusSecure = ConnectionInactive;
  connectionStatus StatusTelegram = ConnectionInactive;

  WiFiClient wifiClient; ///< WifiClient object

  PubSubClient mqttClient; ///< PubSubClient object

  BearSSL::WiFiClientSecure secureClient;
  BearSSL::X509List cert;
  BearSSL::Session session; // session cache used to remember secret keys established with clients, to support session resumption.

  /**
   * @brief WifiMessaging constructor
   */
  WifiMessaging(uint16_t connectionServices);

  /**
   * @brief WifiMessaging constructor with MQTT
   */
  WifiMessaging(uint16_t connectionServices, MQTT_CALLBACK_SIGNATURE);

  /**
   * @brief Congruent combination of connection services
   * 
   *  WiFi
   *   |-- MQTT (Insecure)
   *   |-- NTP (Time)
   *       |-- WiFiSecure
   *           |-- Telegram
   */
  uint16_t CheckConnectionServices(uint16_t connectionServices);

  /**
   * @brief Connect to WiFi
   */
  void connectToWiFi();

  /**
   * @brief Disconnect from WiFi
   */
  void disconnectFromWiFi();

  /**
   * @brief Event WiFi Station connected
   */
  void onSTAConnected(const WiFiEventStationModeConnected &e /*String ssid, uint8 bssid[6], uint8 channel*/);

  /**
   * @brief Event WiFi Station disconnected
   */
  void onSTADisconnected(const WiFiEventStationModeDisconnected &e /*String ssid, uint8 bssid[6], WiFiDisconnectReason reason*/);

  /**
   * @brief Event Got IP from WiFi Station
   */
  void onSTAGotIP(const WiFiEventStationModeGotIP &e /*IPAddress ip, IPAddress mask, IPAddress gw*/);

  /**
   * @brief macId as 12 hexnumber
   */
  String macId();

  /**
   * @brief Local time
   */
  struct tm Timeinfo(struct tm &timeinfo);

  /**
   * @brief Initialise MQTT
   */
  void InitialiseMQTT(MQTT_CALLBACK_SIGNATURE);

  /**
   * @brief Connect to MQTT server
   */
  void ConnectToMqtt();

  /**
   * @brief send Telegram message
   */
  bool sendMessage(const String &text, const String &parse_mode);

private:
  uint16_t _connectionServices = ServiceWifi | ServiceNTP | ServiceSecure | ServiceMQTT | ServiceTelegram;

  // WiFi
  WiFiEventHandler e1;
  WiFiEventHandler e2;
  WiFiEventHandler e4;

  UniversalTelegramBot bot;

  /**
   * @brief Initialise WiFi
   */
  void InitialiseWiFi();

  /**
   * @brief Initialise NTP
   */
  void InitialiseNTP();

  /**
   * @brief Initialise Secure
   */
  void InitialiseSecure();

  /**
   * @brief Initialise Telegram
   */
  void InitialiseTelegram();
};

#endif
