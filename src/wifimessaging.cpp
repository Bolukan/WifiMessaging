#include "wifimessaging.h"

// TIME
#define TIME_NTPSERVER_1 "nl.pool.ntp.org"
#define TIME_NTPSERVER_2 "pool.ntp.org"
#define TIME_ENV_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

// ****************************************************************************
// **                          Constructors                                  **
// ****************************************************************************

/**
 * @brief static pointer used for handling events
 * 
 */
WifiMessaging* WifiMessaging::_wifimessaging = nullptr;

/**
 * @brief Construct a new Wifi Messaging object
 * 
 * @param wifi_ssid 
 * @param wifi_password 
 */
WifiMessaging::WifiMessaging(const char *wifi_ssid, const char *wifi_password)
    : wifi_ssid(wifi_ssid), wifi_password(wifi_password) {

  _wifimessaging = this;
#ifdef ESP8266
  cert = CERTIFICATE_ROOT;  // Initialize cert in the constructor body
#endif
  // Force NTP and WiFi as minimal services
  AddConnectionService(WifiMessaging::connectionService::ServiceNTP);

  // Initialise WiFi: WiFi off and events set
  InitialiseWiFi();
}

/***
 * @brief Reference used for handling events
*/
WifiMessaging &WifiMessaging::instance() { 
  return *_wifimessaging;
}

// ****************************************************************************
// **                          WIFI events                                   **
// ****************************************************************************

#ifdef ESP8266

void WifiMessaging::onSTAConnected(const WiFiEventStationModeConnected &e /*String ssid, uint8 bssid[6], uint8 channel*/) {
  DEBUG_WIFIMESSAGING_PRINTF(
      "WiFi Connected: SSID %s @ BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Channel "
      "%d\n",
      e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3],
      e.bssid[4], e.bssid[5], e.channel);
}

void WifiMessaging::onSTADisconnected(const WiFiEventStationModeDisconnected &e /*String ssid, uint8 bssid[6], WiFiDisconnectReason reason*/) {
  // Reason:
  // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h
  DEBUG_WIFIMESSAGING_PRINTF(
      "WiFi Disconnected: SSID %s BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Reason "
      "%d\n",
      e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3],
      e.bssid[4], e.bssid[5], e.reason);
  StatusWiFi = ConnectionInactive;
}

void WifiMessaging::onSTAGotIP(const WiFiEventStationModeGotIP &e /*IPAddress ip, IPAddress mask, IPAddress gw*/) {
  DEBUG_WIFIMESSAGING_PRINTF(
      "WiFi GotIP: localIP %s SubnetMask %s GatewayIP %s\n",
      e.ip.toString().c_str(), e.mask.toString().c_str(),
      e.gw.toString().c_str());
  StatusWiFi = ConnectionActiveNew;
}

#elif ESP32

// https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/esp32s2/include/esp_event/include/esp_event_legacy.h
void WifiMessaging::wifi_event_handler_static(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  switch (event_id) {
    case SYSTEM_EVENT_STA_CONNECTED:
      return WifiMessaging::instance().onSTAConnected(arg, event_base, event_id, event_data);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      return WifiMessaging::instance().onSTADisconnected(arg, event_base, event_id, event_data);
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      return WifiMessaging::instance().onSTAGotIP(arg, event_base, event_id, event_data);
      break;
    default:
      DEBUG_WIFIMESSAGING_PRINTF("Other WiFi event");
      return;
      break;
  }
}

void WifiMessaging::onSTAConnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  /* 
    uint8_t ssid[32];           // SSID of connected AP
    uint8_t ssid_len;           // SSID length of connected AP
    uint8_t bssid[6];           // BSSID of connected AP
    uint8_t channel;            // channel of connected AP
    wifi_auth_mode_t authmode;  // authentication mode used by AP
  */
  wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;

  DEBUG_WIFIMESSAGING_PRINTF(
      "WiFi Connected: SSID %.*s @ BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Channel %d\n",
      event->ssid_len, event->ssid, 
      event->bssid[0], event->bssid[1], event->bssid[2], event->bssid[3], event->bssid[4], event->bssid[5], 
      event->channel
  );
}

void WifiMessaging::onSTADisconnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  /* 
    uint8_t ssid[32];         // SSID of disconnected AP
    uint8_t ssid_len;         // SSID length of disconnected AP
    uint8_t bssid[6];         // BSSID of disconnected AP
    uint8_t reason;           // reason of disconnection
    int8_t  rssi;             // rssi of disconnection
  */
  wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;

  DEBUG_WIFIMESSAGING_PRINTF(
      "WiFi Disconnected: SSID %.*s BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Reason %d\n",
      event->ssid_len, event->ssid,
      event->bssid[0], event->bssid[1], event->bssid[2], event->bssid[3], event->bssid[4], event->bssid[5],
      event->reason
  );
  
  StatusWiFi = ConnectionInactive;

}

void WifiMessaging::onSTAGotIP(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  /*
    int if_index;                    //< Interface index for which the event is received (left for legacy compilation)
    esp_netif_t *esp_netif;          //< Pointer to corresponding esp-netif object
    esp_netif_ip_info_t ip_info;     //< IP address, netmask, gatway IP address
    bool ip_changed;                 //< Whether the assigned IP has changed or not

  */
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

  uint32_t ip = event->ip_info.ip.addr;
  uint32_t nm = event->ip_info.netmask.addr;
  uint32_t gw = event->ip_info.gw.addr;

  DEBUG_WIFIMESSAGING_PRINTF(
      "WiFi GotIP: localIP %d.%d.%d.%d SubnetMask %d.%d.%d.%d GatewayIP %d.%d.%d.%d\n",
      (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, 
      (nm >> 24) & 0xFF, (nm >> 16) & 0xFF, (nm >> 8) & 0xFF, nm & 0xFF, 
      (gw >> 24) & 0xFF, (gw >> 16) & 0xFF, (gw >> 8) & 0xFF, gw & 0xFF
  );
  
  StatusWiFi = ConnectionActiveNew;

}

#endif

void WifiMessaging::SetMQTT(const char *mqtt_host, uint16_t mqqt_port,
                            MQTT_CALLBACK_SIGNATURE) {
  AddConnectionService(WifiMessaging::connectionService::ServiceMQTT);
  this->mqqt_hostdomain = mqtt_host;
  this->mqtt_hostip = IPAddress(0, 0, 0, 0);
  this->mqqt_port = mqqt_port;
  InitialiseMQTT(callback);
}

void WifiMessaging::SetMQTT(IPAddress mqtt_hostip, uint16_t mqqt_port,
                            MQTT_CALLBACK_SIGNATURE) {
  AddConnectionService(WifiMessaging::connectionService::ServiceMQTT);
  this->mqqt_hostdomain = nullptr;
  this->mqtt_hostip = mqtt_hostip;
  this->mqqt_port = mqqt_port;
  InitialiseMQTT(callback);
}

//    bot(TELEGRAM_BOT, secureClient)
void WifiMessaging::SetTelegram(const char *telegram_bot,
                                const char *telegram_chat_id) {
  AddConnectionService(WifiMessaging::connectionService::ServiceTelegram);
  this->telegram_bot = telegram_bot;
  this->telegram_chat_id = telegram_chat_id;
  this->bot = new UniversalTelegramBot(telegram_bot, this->secureClient);
}

void WifiMessaging::loop() {
  // New WiFi
  if (StatusWiFi == ConnectionActiveNew) {
    StatusWiFi = ConnectionActive;
    if ((connectionServices & ServiceMQTT) && (StatusMQTT < ConnectionActive))
      ConnectToMqtt();
    if ((connectionServices & ServiceNTP) && (StatusNTP < ConnectionActive))
      InitialiseNTP();
  }

  // New NTP
  if (StatusNTP == ConnectionActiveNew) {
    StatusNTP = ConnectionActive;
    if ((connectionServices & ServiceSecure) &&
        (StatusSecure < ConnectionActive)) {
      InitialiseSecure();
    }
  }

  // New Secure
  if (StatusSecure == ConnectionActiveNew) {
    StatusSecure = ConnectionActive;
    if ((connectionServices & ServiceTelegram) &&
        (StatusTelegram < ConnectionActive)) {
      InitialiseTelegram();
    }
  }
}

uint16_t WifiMessaging::AddConnectionService(uint16_t connectionService) {
  this->connectionServices |= connectionService;

  // include Secure to use Telegram
  if (this->connectionServices & ServiceTelegram)
    this->connectionServices |= ServiceSecure;

  // include WiFi and NTP to use Secure
  if (this->connectionServices & ServiceSecure)
    this->connectionServices |= (ServiceWifi | ServiceNTP);

  // include WiFi to use NTP
  if (this->connectionServices & ServiceNTP)
    this->connectionServices |= ServiceWifi;

  // include Wifi to use MQTT
  if (connectionServices & ServiceMQTT) this->connectionServices |= ServiceWifi;

  return this->connectionServices;
}

/**
 * @brief Initialise WiFi: WiFi off and events set
 * 
 */
void WifiMessaging::InitialiseWiFi() {
#ifdef ESP8266

  if (WiFi.getMode() != WIFI_OFF) {
    // 2021-06-20 shutdown function changed, bodge change applied
    WiFi.mode(WIFI_OFF);
    // Set WiFi Off
    if (WiFi.getMode() != WIFI_OFF) {
      DEBUG_WIFIMESSAGING_PRINTF("WIFI not OFF after shutdown\n");
      WiFi.persistent(true);
      WiFi.setAutoConnect(false);    // do not automatically connect on power on
                                     // to the last used access point
      WiFi.setAutoReconnect(false);  // attempt to reconnect to an access point
                                     // in case it is disconnected
      WiFi.disconnect(/* wifioff */ true);
      delay(1);
      WiFi.mode(WIFI_OFF);
      WiFi.persistent(false);
    }
    WiFi.forceSleepBegin();
    delay(1);
  }

  e1 = WiFi.onStationModeConnected(
      std::bind(&WifiMessaging::onSTAConnected, this, std::placeholders::_1));
  e2 = WiFi.onStationModeDisconnected(std::bind(
      &WifiMessaging::onSTADisconnected, this, std::placeholders::_1));
  e4 = WiFi.onStationModeGotIP(
      std::bind(&WifiMessaging::onSTAGotIP, this, std::placeholders::_1));

#elif ESP32

  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.disconnect(true);  // Disconnect and set WiFi mode to OFF
  }
  
  esp_event_handler_instance_register(WIFI_EVENT, SYSTEM_EVENT_STA_CONNECTED,
                                      &wifi_event_handler_static, NULL, &wifi_event_handler_stationConnected);
  esp_event_handler_instance_register(WIFI_EVENT, SYSTEM_EVENT_STA_DISCONNECTED,
                                      &wifi_event_handler_static, NULL, &wifi_event_handler_stationDisconnected);
  esp_event_handler_instance_register(WIFI_EVENT, SYSTEM_EVENT_STA_GOT_IP,
                                      &wifi_event_handler_static, NULL, &wifi_event_handler_stationGotIP);

#endif

  DEBUG_WIFIMESSAGING_PRINTF("Initialised WiFi ...\n");
}

void WifiMessaging::InitialiseMQTT(MQTT_CALLBACK_SIGNATURE) {
  mqttClient.setClient(this->wifiClient);
  mqttClient.setServer(this->mqqt_hostdomain, this->mqqt_port);
  mqttClient.setCallback(callback);
  DEBUG_WIFIMESSAGING_PRINTF("Initialised MQTT ...\n");
}

void WifiMessaging::InitialiseNTP() {
  StatusNTP = ConnectionInBetween;
  configTime(0, 0, TIME_NTPSERVER_1, TIME_NTPSERVER_2);
  setenv("TZ", TIME_ENV_TZ, /*overwrite*/ 1);
  tzset();
#ifdef ESP8266
  timechecker.attach_ms(500, std::bind(&WifiMessaging::checkNTP, this));
#elif ESP32
  timechecker.attach_ms(500, +[](WifiMessaging *instance) { instance->checkNTP(); }, this);
#endif
  DEBUG_WIFIMESSAGING_PRINTF("Initialised NTP...\n");
}

void WifiMessaging::InitialiseSecure() {
#ifdef ESP8266
  secureClient.setSession(&session);  // certificate session to have more
                                      // performance with subsequent calls
  secureClient.setTrustAnchors(&cert);
#elif ESP32
  secureClient.setCACert(CERTIFICATE_ROOT);
#endif
  DEBUG_WIFIMESSAGING_PRINTF("Initialised Secure ...\n");
}

void WifiMessaging::InitialiseTelegram() {
  DEBUG_WIFIMESSAGING_PRINTF("Initialised Telegram ...\n");
}

// ********************  WIFI  ********************

void WifiMessaging::connectToWiFi() {
  DEBUG_WIFIMESSAGING_PRINTF("Connect to WiFi %s ...\n", this->wifi_ssid);
  StatusWiFi = ConnectionInBetween;

#ifdef ESP8266
  // switch on the WiFi radio
  WiFi.forceSleepWake();
  delay(1);
  // Connect to wifi
  WiFi.persistent(false);
  // WiFi.setOutputPower(10);  // reduce RF output power, increase if it won't
  // connect
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(this->wifi_ssid, this->wifi_password);
}

void WifiMessaging::disconnectFromWiFi() {
  DEBUG_WIFIMESSAGING_PRINTF("Disconnect from WiFi ...\n");
  StatusWiFi = ConnectionInBetween;
  // Disconnect to wifi
  WiFi.disconnect(true);
  delay(1);
#ifdef ESP822
  // switch off the WiFi radio
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(5);
#endif
}

// ********************  MQTT  ********************

String WifiMessaging::macId() {
  uint8_t mac[6];
  char macStr[13] = {0};
#ifdef ESP8266
  wifi_get_macaddr(STATION_IF, mac);
#elif ESP32
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
#endif
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
  return String(macStr);
}

void WifiMessaging::ConnectToMqtt() {
  StatusMQTT = ConnectionInBetween;
  String clientId = "ESP-" + macId();

  if (!mqttClient.connected()) {
    if (mqttClient.connect(clientId.c_str())) {
      StatusMQTT = ConnectionActive;
      DEBUG_WIFIMESSAGING_PRINTF("Connected to MQTT as %s\n", clientId.c_str());
    } else {
      DEBUG_WIFIMESSAGING_PRINTF("MQTT connection failed\n");
    }
  }
}

// ********************  NTP  ********************

void WifiMessaging::checkNTP() {
  time_t now = time(nullptr);

  if (StatusNTP < ConnectionActive && now > 24 * 3600) {
    StatusNTP = ConnectionActiveNew;
    timechecker.detach();

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    DEBUG_WIFIMESSAGING_PRINTF("Localtime: %s", asctime(&timeinfo));
  }
}

// ********************  TELEGRAM  ********************

bool WifiMessaging::sendMessage(const String &text, const String &parse_mode) {
  if (StatusTelegram == ConnectionActive) {
    return this->bot->sendMessage(this->telegram_chat_id, text, parse_mode);
  } else {
    return false;
  }
}