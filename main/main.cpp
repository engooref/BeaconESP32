#include "App.h"
#include "sdkconfig.h"

#define INCLUDE_vTaskDelay 1

App* pApp;
void Init() {
    pApp = new App; 
    delete pApp; 
}

extern "C" void app_main()
{
    ESP_LOGI("MAIN", "LANCEMENT DU PROGRAMME");
    Init();
    ESP_LOGI("MAIN", "ARRET DU PROGRAMME");
}