#include "Kernel.h"

#include <iostream>
#include <cstdlib>
#include <ctime>

#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "sdkconfig.h"

#include <vector>

using namespace std;


const string Kernel::TAG = "Kernel";

Kernel::Kernel() :
    m_pGpio(nullptr),
    m_pSandGlass(nullptr)
#ifdef CONFIG_I2C
    ,m_pI2C(nullptr)
#endif

#ifdef CONFIG_UART
    ,m_pUart(nullptr)
#endif 

#ifdef CONFIG_BLUETOOTH
    ,m_pBluetooth(nullptr)
#endif
{
    try {
        ESP_LOGI(TAG.c_str(), "Creation de la classe Kernel");
        
#ifdef CONFIG_SPIFFS
        ESP_LOGI(TAG.c_str(), "Initialisation SPIFFS");
        ESP_ERROR_CHECK(InitSpiffs());
        ESP_LOGI(TAG.c_str(), "Initialisation SPIFFS terminer");
#endif

        ESP_LOGI(TAG.c_str(), "Initialisation GPIO");
        m_pGpio = new Gpio();
        ESP_LOGI(TAG.c_str(), "Initialisation GPIO terminer");

        ESP_LOGI(TAG.c_str(), "Initialisation SandGlass");
        int seq[] = {250, 100, 250, 100};
        m_pSandGlass = new SandGlass(m_pGpio, SandGlass::CreateSequence(4, seq));
        ESP_LOGI(TAG.c_str(), "Initialisation SandGlass terminer");

#ifdef CONFIG_I2C
        ESP_LOGI(TAG.c_str(), "Initialisation de l'I2C");
        m_pI2C = new I2C;
        ESP_LOGI(TAG.c_str(), "Initialisation de l'I2C terminé");
#endif

#ifdef CONFIG_UART
        ESP_LOGI(TAG.c_str(), "Initialisation de l'UART");
        m_pUart = new UART();
        ESP_LOGI(TAG.c_str(), "Initialisation de l'UART terminé");
#endif

#ifdef CONFIG_BLUETOOTH
        ESP_LOGI(TAG.c_str(), "Initialisation du Bluetooth");
        m_pBluetooth = new BluetoothManager();
        ESP_ERROR_CHECK_WITHOUT_ABORT(m_pBluetooth->init());
        ESP_LOGI(TAG.c_str(), "Initialisation du Bluetooth terminé");
#endif

        ESP_LOGI(TAG.c_str(), "Initialisation des composants terminé");
        
    } catch (const runtime_error &e) {
        ESP_LOGE(TAG.c_str(), "Arret");
        ESP_LOGE(TAG.c_str(), "%s", e.what());
    }
}

Kernel::~Kernel() {
    ESP_LOGI(TAG.c_str(), "Destruction de la classe Kernel");

#ifdef CONFIG_UART
    if(m_pUart)      {delete m_pUart; m_pUart = nullptr;}
#endif

#ifdef CONFIG_I2C
    if(m_pI2C)       {delete m_pI2C; m_pI2C = nullptr;}
#endif

#ifdef CONFIG_BLUETOOTH
    if(m_pBluetooth)       {delete m_pBluetooth; m_pBluetooth = nullptr;}
#endif

    if(m_pSandGlass) {delete m_pSandGlass; m_pSandGlass = nullptr;}
    if(m_pGpio)      {delete m_pGpio; m_pGpio = nullptr;}

}


#ifdef CONFIG_SPIFFS
esp_err_t Kernel::InitSpiffs(){

    esp_vfs_spiffs_conf_t conf = 
    {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) 
        {
            ESP_LOGE(CONFIG_TAG_SPIFFS, "Failed to mount or format filesystem !");
        } else if (ret == ESP_ERR_NOT_FOUND) 
        {
            ESP_LOGE(CONFIG_TAG_SPIFFS, "Failed to find SPIFFS partition !");
        } else 
        {
            ESP_LOGE(CONFIG_TAG_SPIFFS, "Failed to initialize SPIFFS (%s) !", esp_err_to_name(ret));
        }
    }
    return ret;
}
#endif