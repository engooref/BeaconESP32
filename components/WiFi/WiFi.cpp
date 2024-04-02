#include "WiFi.h"

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sys.h"

using namespace std;

esp_netif_t* WiFi::s_pNetif = nullptr;
const string WiFi::TAG = "WiFi";

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

#define MAX_RETRY 5

/**
 *  @def Swap2Bytes(val)
 * 
 *  @param val valeur à permuté
 *
 *  @brief Permet de permuter entre big endians et little endians
 */
#define Swap2Bytes(val)  val = ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )


/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

int nbRetry = 0;
/** @brief Constructeur de la classe WiFi */
WiFi::WiFi() :
    m_pParamJSON(nullptr)
{
    ESP_LOGI(TAG.c_str(), "Creation de la classe WiFi");

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_pNetif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
   ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WiFiEvent,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &WiFiEvent,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta {        
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    ESP_LOGI(TAG.c_str(), "Ouverture du fichier des reseaux WiFi");
    string nameFile;
    nameFile += string("/spiffs/") +  CONFIG_FILE_NAME;
    ifstream fileWiFi(nameFile); 
    if( !fileWiFi ){
        ESP_LOGE(TAG.c_str(), "Le fichier des reseaux WiFi n'a pas pu être ouvert, code erreur: %d - %s", errno, strerror(errno));
        return;
    }

    std::stringstream buf;
    buf << fileWiFi.rdbuf();
    m_pParamJSON = new ClaJSON(buf.str());
    fileWiFi.close();
    vector<ClaJSON*> vecSta = m_pParamJSON->GetTab("WiFiSta");
    bool isConnected = false;

    for(auto staWiFi = vecSta.begin(); staWiFi < vecSta.end() && !isConnected; staWiFi++){
        s_retry_num = 0;

        nbRetry = (*staWiFi)->GetInt("Retry");
        strcpy((char*)wifi_config.sta.ssid, (*staWiFi)->GetStr("SSID").c_str());
        strcpy((char*)wifi_config.sta.password, (*staWiFi)->GetStr("MDP").c_str());
        wifi_config.sta.threshold.authmode = (string((const char*)wifi_config.sta.password).length() > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG.c_str(), "SSID: %s\tPassword: %s", wifi_config.sta.ssid, wifi_config.sta.password);

        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
        * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
        * happened. */

        if ((isConnected = (bits & WIFI_CONNECTED_BIT))) {
            ESP_LOGI(TAG.c_str(), "Connecter au réseau SSID: %s",
                    wifi_config.sta.ssid);
            m_configWiFi = wifi_config;
        } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGI(TAG.c_str(), "Erreur dans la connexion");
            if(staWiFi < vecSta.end() - 1) ESP_LOGI(TAG.c_str(), "Test de la connexion avec le prochain réseau sauvegarder");
            ESP_ERROR_CHECK(esp_wifi_stop());
        } else {
            throw runtime_error("UNEXPECTED EVENT");
        }
    }


    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);

    if(!isConnected){
        if(m_pParamJSON != nullptr) {delete m_pParamJSON; m_pParamJSON = nullptr;}

        throw runtime_error("Impossible de se connecter au réseau WiFi enregistré");
    }

}

/** @brief Destructeur de la classe WiFi */
WiFi::~WiFi(){
    if(m_pParamJSON != nullptr) {delete m_pParamJSON; m_pParamJSON = nullptr;}
}


void WiFi::GetIp(char buf[16]) {
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    esp_netif_ip_info_t ip_info;
    
    if(mode == WIFI_MODE_STA){
        esp_netif_get_ip_info(s_pNetif, &ip_info);    
    } else {
    }
    
    sprintf(buf, IPSTR, IP2STR(&ip_info.ip));
    
}


/**
 *  @brief Gestion des événements WiFi.
 *
 *  Fonction statique appelée à chaque événement WiFi permettant de gérer les événements WiFi.
 * 
 *  @param arg : argument de la fonction
 *  @param event_base : événement enregistré sur le WiFi
 *  @param event_id : identifiant de l'événement
 *  @param event_date : données de l'événément
 */
void WiFi::WiFiEvent(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num != nbRetry) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG.c_str(), "Nouvel tentative de connexion");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG.c_str(),"Connexion au réseau échoué");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
