#include "Bluetooth.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "BluetoothManager";

// Initialisation du handle de connexion à une valeur invalide.
uint32_t BluetoothManager::spp_conn_handle = 0xFFFFFFFF;
BluetoothManager::DataReceivedCallback BluetoothManager::dataReceivedCallback = nullptr;
BluetoothManager* BluetoothManager::instance = nullptr;
bool BluetoothManager::state = false;

BluetoothManager::BluetoothManager() :
    m_pObj(nullptr)
{
    instance = this;
    // Constructeur (si besoin, ajouter d'autres initialisations)
}

BluetoothManager::~BluetoothManager() {
    // Destructeur : ici, ajoutez le nettoyage nécessaire si vous le souhaitez
}

esp_err_t BluetoothManager::init() {
    // --- NVS init (inchangé) ---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS Flash init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // --- Controller init ---
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_LOGI(TAG, "Bluetooth Mode config: %d", bt_cfg.mode);

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Si vous n'utilisez pas BLE, libérez la mémoire (optionnel) :
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    // Activez le contrôleur en CLASSIC_BT (NE PAS passer par BLE seulement)
    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Controller enable CLASSIC_BT failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // --- Bluedroid init & enable ---
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // --- SPP registration & enhanced init ---
    ret = esp_spp_register_callback(BluetoothManager::sppCallback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPP callback registration failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Option : définir des paramètres dans spp_cfg si nécessaire
    esp_spp_cfg_t spp_cfg = {};
    ret = esp_spp_enhanced_init(&spp_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPP enhanced init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Bluetooth CLASSIC initialisé, en attente de SPP_INIT_EVT...");
    return ESP_OK;
}


void BluetoothManager::waitForConnectionLoop() {
    // Boucle infinie pour laisser FreeRTOS gérer les événements en arrière-plan
    while (!state) {
        
        ESP_LOGI(TAG, "%s", ((state) ? " true" : "false"));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

esp_err_t BluetoothManager::Send(const std::string data) {
    if (spp_conn_handle == 0xFFFFFFFF) {
        ESP_LOGE(TAG, "Pas de connexion SPP établie, impossible d'envoyer des données");
        throw;
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = esp_spp_write(spp_conn_handle, data.length(), (uint8_t*)data.c_str());
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Données envoyées: %.*s", data.length(), data.c_str());
    } else {
        ESP_LOGE(TAG, "Erreur lors de l'envoi des données: %s", esp_err_to_name(err));
    }
    return err;
}

void BluetoothManager::sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(TAG, "SPP initialisé. Configuration du nom et démarrage du serveur.");
            // Définir le nom du périphérique et passer en mode connectable/découvrable.
            esp_bt_gap_set_device_name("ESP32_BLUETOOTH");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            // Démarrer le serveur SPP en mode esclave.
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "ESP32_SPP_SERVER");
            break;
            
        case ESP_SPP_START_EVT:
            ESP_LOGI(TAG, "Serveur SPP démarré, en attente de connexion...");
            break;
            
        case ESP_SPP_SRV_OPEN_EVT:
            // Une connexion SPP a été établie, stocker le handle de connexion.
            spp_conn_handle = param->open.handle;
            ESP_LOGI(TAG, "Connexion SPP établie (handle: %d)", (int)spp_conn_handle);
            state = true;

            break;
            
        case ESP_SPP_DATA_IND_EVT:
            ESP_LOGI(TAG, "Données reçues (handle=%d, len=%d)", (int)param->data_ind.handle, (int)param->data_ind.len);
            // Exemple de traitement : afficher les données reçues en hexadécimal.
            ESP_LOG_BUFFER_HEX(TAG, param->data_ind.data, param->data_ind.len);
            // Vous pouvez ajouter ici un traitement personnalisé (par exemple, stocker ou traiter la donnée).
            if (dataReceivedCallback != nullptr) {
                dataReceivedCallback(instance->m_pObj, param->data_ind.data, param->data_ind.len);
            }
            break;

        case ESP_SPP_CLOSE_EVT:
            ESP_LOGI(TAG, "Connexion SPP fermée");
            spp_conn_handle = 0xFFFFFFFF;
            break;
            
        default:
            break;
    }
}

void BluetoothManager::setDataReceivedCallback(DataReceivedCallback callback, void* pObj) {
    dataReceivedCallback = callback; 
    m_pObj = pObj;
}
