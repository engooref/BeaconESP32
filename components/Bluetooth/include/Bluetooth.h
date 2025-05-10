#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include "esp_err.h"
#include "esp_spp_api.h"    // Pour les définitions SPP
#include <iostream>

class BluetoothManager {
private:
    void* m_pObj;
public:
    BluetoothManager();
    ~BluetoothManager();

    /**
     * Initialise la pile Bluetooth (NVS, contrôleur, Bluedroid, SPP).
     * Retourne ESP_OK en cas de succès, sinon une erreur.
     */
    esp_err_t init();

    /**
     * Boucle infinie pour laisser le système FreeRTOS gérer les événements Bluetooth.
     */
    void waitForConnectionLoop();

    /**
     * Envoie des données sur la connexion SPP établie.
     *
     * @param data chaine a envoyer.
     * @return ESP_OK en cas de succès, sinon une erreur.
     */
    esp_err_t Send(const std::string data);

    /**
     * Callback SPP statique appelée par la pile Bluetooth pour gérer les événements.
     */
    static void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

    // Définition du type de callback pour la réception des données
    typedef void (*DataReceivedCallback)(void* pObj, const uint8_t* data, uint16_t len);

    /**
     * Définit la fonction callback à appeler lors de la réception de données.
     */
    void setDataReceivedCallback(DataReceivedCallback callback, void* pObj);
   
private:
    // Variable statique pour stocker le callback de réception
    static DataReceivedCallback dataReceivedCallback;
    
    // Nous utilisons un handle statique pour stocker la connexion SPP active.
    // La valeur 0xFFFFFFFF signale qu'aucune connexion n'est établie.
    static uint32_t spp_conn_handle;

    // **Ajout d'une variable statique pour pointer vers l'instance active**
    static BluetoothManager* instance;

    static bool state;
};

#endif // BLUETOOTH_MANAGER_H
