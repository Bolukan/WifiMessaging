#include "wifimessaging.h"

// TIME
#define TIME_NTPSERVER_1 "nl.pool.ntp.org"
#define TIME_NTPSERVER_2 "pool.ntp.org"
#define TIME_ENV_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

WifiMessaging::WifiMessaging(uint16_t connectionServices)
    : cert(CERTIFICATE_ROOT),
      _connectionServices(CheckConnectionServices(connectionServices)),
      bot(TELEGRAM_BOT, secureClient)
{
  DEBUG_WIFIMESSAGING_BEGIN();

  if (_connectionServices & ServiceWifi)
  {
    StatusWiFi = ConnectionInactive;
    InitialiseWiFi();
  }

  if (_connectionServices & ServiceMQTT)
  {
    StatusMQTT = ConnectionInactive;
    //InitialiseMQTT();
  }

  if (_connectionServices & ServiceNTP)
  {
    StatusNTP = ConnectionInactive;
    InitialiseNTP();
  }

  // Started after NTP
  if (_connectionServices & ServiceSecure)
  {
    StatusSecure = ConnectionInactive;
    // InitialiseSecure();  // Started after NTP
  }

  if (_connectionServices & ServiceTelegram)
  {
    StatusTelegram = ConnectionInactive;
    // InitialiseTelegram();  // Started after SecureClient
  }

  DEBUG_WIFIMESSAGING_PRINTF("WifiMessaging initialised\n");
}

WifiMessaging::WifiMessaging(uint16_t connectionServices, MQTT_CALLBACK_SIGNATURE)
    : WifiMessaging(connectionServices)
{

  InitialiseMQTT(callback);
}

uint16_t WifiMessaging::CheckConnectionServices(uint16_t connectionServices)
{
  if (connectionServices & ServiceTelegram)
    connectionServices |= ServiceSecure;
  if (connectionServices & ServiceSecure)
    connectionServices |= (ServiceWifi | ServiceNTP);
  if (connectionServices & ServiceNTP)
    connectionServices |= ServiceWifi;
  if (connectionServices & ServiceMQTT)
    connectionServices |= ServiceWifi;
  return connectionServices;
}

void WifiMessaging::InitialiseWiFi()
{
  if (WiFi.getMode() != WIFI_OFF)
  {
    DEBUG_WIFIMESSAGING_PRINTF("WIFI NOT OFF: SHUTDOWN\n");
    WiFi.mode(WIFI_SHUTDOWN);
    // Set WiFi Off
    if (WiFi.getMode() != WIFI_OFF)
    {
      DEBUG_WIFIMESSAGING_PRINTF("WIFI NOT OFF AFTER SHUTDOWN\n");
      WiFi.persistent(true);
      WiFi.setAutoConnect(false);   // do not automatically connect on power on to the last used access point
      WiFi.setAutoReconnect(false); // attempt to reconnect to an access point in case it is disconnected
      WiFi.disconnect(/* wifioff */ true);
      delay(1);
      WiFi.mode(WIFI_OFF);
      WiFi.persistent(false);
    }
    WiFi.forceSleepBegin();
    delay(1);
  }
  e1 = WiFi.onStationModeConnected(std::bind(&WifiMessaging::onSTAConnected, this, std::placeholders::_1));
  e2 = WiFi.onStationModeDisconnected(std::bind(&WifiMessaging::onSTADisconnected, this, std::placeholders::_1));
  e4 = WiFi.onStationModeGotIP(std::bind(&WifiMessaging::onSTAGotIP, this, std::placeholders::_1));
}

void WifiMessaging::InitialiseNTP()
{
  DEBUG_WIFIMESSAGING_PRINTF("Initialise NTP ...\n");
  configTime(0, 0, TIME_NTPSERVER_1, TIME_NTPSERVER_2);
  setenv("TZ", TIME_ENV_TZ, /*overwrite*/ 1);
  tzset();
}

void WifiMessaging::InitialiseSecure()
{
  DEBUG_WIFIMESSAGING_PRINTF("Initialise Secure ...\n");
  // BearSSL::WiFiClientSecure secureClient;
  // BearSSL::X509List cert(CERTIFICATE_ROOT);
  // BearSSL::Session session; // session cache used to remember secret keys established with clients, to support session resumption.

  secureClient.setSession(&session); // certificate session to have more performance with subsequent calls
  secureClient.setTrustAnchors(&cert);

  StatusSecure = ConnectionActive;

  if ((_connectionServices & ServiceTelegram) && (StatusTelegram != ConnectionActive))
  {
    InitialiseTelegram();
  }
}

void WifiMessaging::InitialiseMQTT(MQTT_CALLBACK_SIGNATURE)
{
  DEBUG_WIFIMESSAGING_PRINTF("Initialise MQTT ...\n");
  mqttClient = PubSubClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void WifiMessaging::InitialiseTelegram()
{
  DEBUG_WIFIMESSAGING_PRINTF("Initialise Telegram (Emtpy) ...\n");
  StatusTelegram = ConnectionActive;
  // bot(TELEGRAM_BOT, secureClient);
}

// ********************  WIFI  ********************

void WifiMessaging::connectToWiFi()
{
  DEBUG_WIFIMESSAGING_PRINTF("Connect to WiFi ... ");
  StatusWiFi = ConnectionInactiveToActive;
  // switch on the WiFi radio
  WiFi.forceSleepWake();
  delay(1);
  // Connect to wifi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  // WiFi.setOutputPower(10);  // reduce RF output power, increase if it won't connect
  // WiFi.config(WIFI_STATICIP, WIFI_GATEWAY, WIFI_SUBNET);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  DEBUG_WIFIMESSAGING_PRINTF("Connected to WiFi\n");
}

void WifiMessaging::disconnectFromWiFi()
{
  DEBUG_WIFIMESSAGING_PRINTF("Disconnect from WiFi ... ");
  StatusWiFi = ConnectionActiveToInactive;
  // Disconnect to wifi
  WiFi.disconnect(true);
  delay(1);
  // switch off the WiFi radio
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(5);
  DEBUG_WIFIMESSAGING_PRINTF("Disconnected from WiFi\n");
}

// More events: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiGeneric.h

void WifiMessaging::onSTAConnected(const WiFiEventStationModeConnected &e /*String ssid, uint8 bssid[6], uint8 channel*/)
{
  DEBUG_WIFIMESSAGING_PRINTF("WiFi Connected: SSID %s @ BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Channel %d\n",
                             e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.channel);
}

void WifiMessaging::onSTADisconnected(const WiFiEventStationModeDisconnected &e /*String ssid, uint8 bssid[6], WiFiDisconnectReason reason*/)
{
  // Reason: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h
  DEBUG_WIFIMESSAGING_PRINTF("WiFi Disconnected: SSID %s BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Reason %d\n",
                             e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.reason);
  StatusWiFi = ConnectionInactive;
}

void WifiMessaging::onSTAGotIP(const WiFiEventStationModeGotIP &e /*IPAddress ip, IPAddress mask, IPAddress gw*/)
{
  DEBUG_WIFIMESSAGING_PRINTF("WiFi GotIP: localIP %s SubnetMask %s GatewayIP %s\n",
                             e.ip.toString().c_str(), e.mask.toString().c_str(), e.gw.toString().c_str());
  StatusWiFi = ConnectionActive;

  if (_connectionServices & ServiceMQTT)
  {
    StatusMQTT = ConnectionInactiveToActive;
    ConnectToMqtt();
  }
}

void WifiMessaging::ConnectToMqtt()
{
  String clientId = "ESP-" + macId();
  if (mqttClient.connect(clientId.c_str()))
  {
    StatusMQTT = ConnectionActive;
    DEBUG_WIFIMESSAGING_PRINTF("Connected to MQTT\n");
  }
  else
  {
    DEBUG_WIFIMESSAGING_PRINTF("Connection to MQTT failed\n");
  }
}

String WifiMessaging::macId()
{
  uint8_t mac[6];
  char macStr[13] = {0};
  wifi_get_macaddr(STATION_IF, mac);
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

struct tm WifiMessaging::Timeinfo(struct tm &timeinfo)
{
  time_t now = time(nullptr);
  localtime_r(&now, &timeinfo);

  if (StatusNTP != ConnectionActive && now > 24 * 3600)
  {
    DEBUG_WIFIMESSAGING_PRINTF("Localtime: %s\n", asctime(&timeinfo));
    StatusNTP = ConnectionActive;

    if ((_connectionServices & ServiceSecure) && (StatusSecure != ConnectionActive))
    {
      StatusSecure = ConnectionInactiveToActive;
      InitialiseSecure();
    }
  }
  return timeinfo;
}

bool WifiMessaging::sendMessage(const String &text, const String &parse_mode)
{
  return bot.sendMessage(TELEGRAM_CHAT_ID, text, parse_mode);
}