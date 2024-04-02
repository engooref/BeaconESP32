#include "App.h"
#include <iostream>

using namespace std;

string App::TAG = "App";

#define GPIO_BUTTON GPIO_NUM_5
#define GPIO_SWITCH GPIO_NUM_18

App::App(){
   
   GetSocket()->ChangeTraitFunc(TraitSocket, this);
   int seq[] = {500, 250, 500, 250};
   GetSandGlass()->AddSequence(SandGlass::CreateSequence(4, seq), true);
   
   GetGPIO()->ChangeMode(GPIO_MODE_INPUT);
   GetGPIO()->AddGPIO(GPIO_BUTTON); 
   GetGPIO()->AddGPIO(GPIO_SWITCH);
   GetGPIO()->ApplySetting();

   Run(); 
}

App::~App(){

}

void App::Run() {
   ESP_LOGI(TAG.c_str(), "Lancement de la boucle");

   ESP_LOGI(TAG.c_str(), "En attente d'une connexion socket");
   while(!GetSocket()->IsConnected()) vTaskDelay(25 / portTICK_PERIOD_MS);


   auto err = [this]{
      GetSocket()->Send("1;3");
      ESP_LOGI(TAG.c_str(), "Retrait de la carte prematurée");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
   };
   
   for(;;) {

      reset = false;
      GetSocket()->Send("1;0");
      ESP_LOGI(TAG.c_str(), "En attente de la carte");
      while(! GetGPIO()->IsHigh(GPIO_SWITCH))
         vTaskDelay(25 / portTICK_PERIOD_MS);
      

      GetSocket()->Send("1;1");
      ESP_LOGI(TAG.c_str(), "Chargement de la carte veuillez patientez");

      for(int x = 0; x < 40; x++) 
         if (! GetGPIO()->IsHigh(GPIO_SWITCH))
            break;
         else
            vTaskDelay(25 / portTICK_PERIOD_MS);

      if (! GetGPIO()->IsHigh(GPIO_SWITCH)) {
         err();
         continue;
      }

      GetSocket()->Send("1;2");
      ESP_LOGI(TAG.c_str(), "Chargement terminé");

      while(! GetGPIO()->IsHigh(GPIO_BUTTON)) {
         if (! GetGPIO()->IsHigh(GPIO_SWITCH)) {
            err();
            break;
         }
      }

      if (GetGPIO()->IsHigh(GPIO_SWITCH)) {
         GetSocket()->Send("1;4");
         ESP_LOGI(TAG.c_str(), "Accès autorisée");
         ESP_LOGI(TAG.c_str(), "En attente du retour de la tablette");
         while(!reset) vTaskDelay(25 / portTICK_PERIOD_MS);
      }

   }
}

void App::TraitSocket(void* pObj, char* buf){
   
   static_cast<App*>(pObj)->reset = (buf[0] == 'O');
}