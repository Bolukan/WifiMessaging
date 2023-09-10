#ifndef WIFIMESSAGING_H
#define WIFIMESSAGING_H

#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>  // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <Ticker.h>  // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/Ticker/src/Ticker.h
#include <WiFiClientSecure.h>  // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h
#include <time.h>  // Arduino library - https://github.com/esp8266/Arduino/blob/master/tools/sdk/libc/xtensa-lx106-elf/include/time.h

#elif ESP32
#include <Ticker.h>  //                   https://github.com/espressif/arduino-esp32/blob/master/libraries/Ticker/src/Ticker.h
#include <WiFi.h>
#include <WiFiClientSecure.h>  // Arduino library - https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiClientSecure/src/WiFiClientSecure.h
#include <time.h>  // https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/esp32/include/newlib/platform_include/time.h

#endif

#include <Certificate_telegram.h>
#include <PubSubClient.h>  // 89              - https://github.com/knolleary/pubsubclient.git
#include <UniversalTelegramBot.h>  // 1262            - https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot.git

// **************************************** DEBUG
// ****************************************

// Uncomment lines below to receive debug messages via Serial
#define DEBUG_WIFIMESSAGING_TO_SERIAL 1

#ifdef DEBUG_WIFIMESSAGING_TO_SERIAL
#define DEBUG_WIFIMESSAGING_BEGIN()  \
  if (!Serial) Serial.begin(115200); \
  Serial.println();
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

// **************************************** CLASS
// **************************************** WifiMessaging(ServiceWifi |
// ServiceNTP | ServiceSecure | ServiceMQTT | ServiceTelegram);
// WifiMessaging.InitialiseMQTT(callback);
// WifiMessaging.connectToWiFi();

/**
 * WiFi and messaging class (h).
 */
class WifiMessaging {
 public:
  enum connectionStatus {
    ConnectionInactive = 0,
    ConnectionInBetween = 1,
    ConnectionActive = 2,
    ConnectionActiveNew = 3
  };

  enum connectionService : uint16_t {
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

  // MQTT
  PubSubClient mqttClient;  ///< PubSubClient object

  /**
   * @brief Construct a new Wifi Messaging object
   *
   * @param wifi_ssid Service Set Identifier
   * @param wifi_password Password of the SSID Network
   */
  WifiMessaging(const char *wifi_ssid, const char *wifi_password);

  /**
   * @brief Set MQTT parameters
   *
   * @param mqtt_host mqtt host
   * @param mqtt_port mqtt port
   * @param callback function for received messages
   */
  void SetMQTT(const char *mqtt_host, uint16_t mqtt_port,
               MQTT_CALLBACK_SIGNATURE);

  /**
   * @brief Set MQTT parameters
   *
   * @param mqtt_hostip
   * @param mqqt_port
   * @param callback function for received messages
   */
  void SetMQTT(IPAddress mqtt_hostip, uint16_t mqqt_port,
               MQTT_CALLBACK_SIGNATURE);

  /**
   * @brief Set the Telegram object
   *
   * @param telegram_bot The telegram bot code
   */
  void SetTelegram(const char *telegram_bot, const char *telegram_chat_id);

  /**
   * @brief Act on situation
   */
  void loop();

  /**
   * @brief Connect to WiFi
   */
  void connectToWiFi();

  /**
   * @brief Disconnect from WiFi
   */
  void disconnectFromWiFi();

  /**
   * @brief macId as 12 hexnumber
   */
  String macId();

  /**
   * @brief send Telegram message
   */
  bool sendMessage(const String &text, const String &parse_mode);

  /**
   * @brief return static object of this class
   *
   * @return WifiMessaging&
   */
  static WifiMessaging &instance();

#ifdef ESP32

  static void wifi_event_handler_static(void *arg, esp_event_base_t event_base,
                                        int32_t event_id, void *event_data);

  /**
   * @brief Event WiFi Station
   */
  void wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

#endif

 private:
  /**
   * @brief Intended connection services, set if constants connection service
   * are set ServiceWifi | ServiceNTP | ServiceSecure | ServiceMQTT |
   * ServiceTelegram
   *
   */
  uint16_t connectionServices = ServiceWifi;

  // WiFi
  const char *wifi_ssid;      ///< Wifi SSID
  const char *wifi_password;  ///< WiFi password
  WiFiClient wifiClient;      ///< WifiClient object
  
  static WifiMessaging *_wifimessaging;
  
#ifdef ESP8266
  WiFiEventHandler e1;  ///< event onStationModeConnected
  WiFiEventHandler e2;  ///< event onStationModeDisconnected
  WiFiEventHandler e4;  ///< event onStationModeGotIP
#elif ESP32
  esp_event_handler_instance_t wifi_event_handler_stationConnected;
  esp_event_handler_instance_t wifi_event_handler_stationDisconnected;
  esp_event_handler_instance_t wifi_event_handler_stationGotIP;
#endif

  // MQTT
  IPAddress mqtt_hostip;
  const char *mqqt_hostdomain;
  uint16_t mqqt_port;

  // NTP
  Ticker timechecker;

#ifdef ESP8266
  // Secure
  BearSSL::WiFiClientSecure secureClient;
  BearSSL::X509List cert;
  BearSSL::Session
      session;  // session cache used to remember secret keys established with
                // clients, to support session resumption.
#elif ESP32
  WiFiClientSecure secureClient;
#endif

  // Telegram
  const char *telegram_bot;
  const char *telegram_chat_id;
  UniversalTelegramBot *bot;

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

  uint16_t AddConnectionService(uint16_t connectionService);

  /**
   * @brief Initialise WiFi: WiFi off and events set
   */
  void InitialiseWiFi();

  /**
   * @brief Initialise MQTT
   */
  void InitialiseMQTT(MQTT_CALLBACK_SIGNATURE);

  /**
   * @brief Connect to MQTT server
   */
  void ConnectToMqtt();

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

#ifdef ESP8266

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

#elif ESP32
  void  onSTAConnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  void  onSTADisconnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  void  onSTAGotIP(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif

  /**
   * @brief Check time received via NTP
   */
  void checkNTP();
};

#endif
