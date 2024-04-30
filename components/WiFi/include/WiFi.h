#ifndef CWIFI_H_
#define CWIFI_H_

#include <string>

#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "ClaJSON.h"

/**
  * @class WiFi
  * @brief Classe utilisé pour le WiFi.
  *
  *  Cette classe gère le WiFi ainsi que les événements liés à celui-ci
  */
class WiFi {
private:
    wifi_config_t m_configWiFi;
    ClaJSON* m_pParamJSON;
public:
    WiFi();
    ~WiFi();


public:  
    static void GetIp(char* buf);
private:
    static void WiFiEvent(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

private:
    static const std::string TAG;
    static esp_netif_t* s_pNetif;

};

#endif //CWIFI_H_