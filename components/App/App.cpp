#include "App.h"
#include <iostream>
#include <sstream>

using namespace std;

string App::TAG = "App";

#define BTN_POUS (gpio_num_t)CONFIG_BTN_POUS
#define BTN_SWITCH (gpio_num_t)CONFIG_BTN_SWITCH
#define LED_RED (gpio_num_t)CONFIG_LED_RED
#define LED_GREEN1 (gpio_num_t)CONFIG_LED_GREEN1
#define LED_GREEN2 (gpio_num_t)CONFIG_LED_GREEN2


App::App() : 
   m_user({"", "", ""})
{
   

   GetBluetooth()->setDataReceivedCallback(App::TraitBluetooth, this);
   
   GetGPIO()->ChangeMode(GPIO_MODE_INPUT);
   GetGPIO()->AddGPIO(BTN_POUS); 
   GetGPIO()->SetPullUp(true);
   GetGPIO()->ApplySetting();

   GetGPIO()->ChangeMode(GPIO_MODE_OUTPUT);
   GetGPIO()->AddGPIO(LED_RED);
   GetGPIO()->AddGPIO(LED_GREEN1);
   GetGPIO()->AddGPIO(LED_GREEN2); 
   GetGPIO()->ApplySetting();
 

   m_pNFC = new NFC();

   uint32_t versiondata = 0;
   // Séquence LED pour attente PN530 (initialisation avec clignotements irréguliers)
   int seq_pn530[] = {400, 550, 250, 700};

   GetSandGlass()->AddSequence(SandGlass::CreateSequence(4, seq_pn530), true);
   ESP_LOGI(TAG.c_str(), "En attente du module de carte");
   // do 
   // {
   //    versiondata = m_pNFC->GetFirmwareVersion();
   //    vTaskDelay(500 / portTICK_PERIOD_MS);
   // } while (!versiondata);

   // // Got ok data, print it out!
   // ESP_LOGI(TAG.c_str(), "Found chip PN5 %x", (unsigned int)(((versiondata >> 24) & 0xFF)));
   // ESP_LOGI(TAG.c_str(), "Firmware ver. %d.%d", (int)(versiondata >> 16) & 0xFF, (int)(versiondata >> 8) & 0xFF);

   // // configure board to read RFID tags
   // m_pNFC->SAMConfig();
   // uint8_t key[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
   // m_pNFC->SetKey(0x00, key);

   Run(); 
}

App::~App(){
   if (m_pNFC) delete m_pNFC;
}

#ifdef CONFIG_PN532_WRITE
uint8_t App::SaveInformationIntoNFC() {
   memset(dataSector, 0, sizeof(dataSector));
   uint8_t deb = 0x04, fin = 0x04;

   // Création des entêtes
   std::vector<HeaderInfo> headers;

   // Lambda pour écrire les données utilisateur dans les secteurs
   auto writeUserData = [this, &headers] (uint8_t& fin) {
      auto fun = [this] (const std::string& str, uint8_t blockDeb) -> uint8_t {
            int lenStr = str.length();
            int nbBlock = (lenStr + BLOCK_SIZE - 1) / BLOCK_SIZE;
            ESP_LOGI(TAG.c_str(), "chaine : %s - length: %d - nbBlock: %d", str.c_str(), lenStr, nbBlock);

            for(int x = 0; x < nbBlock; x++) {
               int sectorIndex = ((blockDeb + x - 1) / (NBBLOCK_WRITEABLE)) - 1;
               int blockIdx = (blockDeb - 1 + x) % (NBBLOCK_WRITEABLE);

               ESP_LOGI(TAG.c_str(), "Ecriture - Secteur: %d - block %d", sectorIndex, blockIdx);

               std::string subStr = str.substr(x * BLOCK_SIZE, BLOCK_SIZE);
               memcpy(dataSector[sectorIndex][blockIdx], subStr.c_str(), subStr.length());
               for (int x = subStr.length(); x < BLOCK_SIZE; x++) 
                  dataSector[sectorIndex][blockIdx][x] = 0xFF;

               esp_log_buffer_hexdump_internal(TAG.c_str(), dataSector[sectorIndex][blockIdx], BLOCK_SIZE, ESP_LOG_INFO);
            }

            
            return blockDeb + nbBlock - 1;
      };

      HeaderInfo header = {1, fin, 0};
      fin = fun(this->m_user.handle, fin);
      header.blockFin = fin;
      headers.push_back(header);

      header.id = 2;
      header.blockDeb = fin + 1;
      fin = fun(this->m_user.pseudoDiscord, header.blockDeb);
      header.blockFin = fin;
      headers.push_back(header);

      header.id = 3;
      header.blockDeb = fin + 1;    
      fin = fun(this->m_user.email, header.blockDeb);
      header.blockFin = fin;
      headers.push_back(header);

      for (const auto& headert : headers) 
         ESP_LOGI(TAG.c_str(), "header.id: %d - header.blockDeb: %d - header.blockFin: %d", headert.id, headert.blockDeb, headert.blockFin);
   };


   // Écriture des données utilisateur et mise à jour des entêtes
   writeUserData(fin);


   // Écriture des entêtes dans le bloc 0x01
   for (int x = 0; x < headers.size(); x++) {
         blockEntete[x * sizeof(uint8_t) * 3] = headers[x].id;
         blockEntete[x * sizeof(uint8_t) * 3 + sizeof(uint8_t)] = headers[x].blockDeb;
         blockEntete[x * sizeof(uint8_t) * 3 + sizeof(uint8_t) * 2] = headers[x].blockFin;
   }

   esp_log_buffer_hexdump_internal(TAG.c_str(), blockEntete, BLOCK_SIZE, ESP_LOG_INFO);

   // Écriture de l'entête dans le bloc 0x01
   if (!m_pNFC->MifareclassicWriteDataBlock(0x01, blockEntete)) return 0;

   // Calcul du nombre de secteurs utilisés
   int usedSectors = (fin / 3) - (deb / 3) + 1;
   ESP_LOGI(TAG.c_str(), "Nombre de secteurs utilisé: %d", usedSectors);

   // Écriture des données utilisateur dans les secteurs
   for (int x = 0; x < usedSectors; x++) {
      if (!m_pNFC->MifareclassicWriteDataSector(x + 1, dataSector[x][0], dataSector[x][1], dataSector[x][2])) return 0;
   }

   return 1;
}
#endif

uint8_t App::RetrieveInformationFromNFC() {
   memset(dataSector, 0, sizeof(dataSector));
   // Lecture des en-têtes
   std::vector<HeaderInfo> headers;

   uint8_t deb = 0xFF, fin = 0x00;

   // Lecture de l'entête depuis le bloc 0x01
   if (!m_pNFC->MifareclassicReadDataBlock(0x01, blockEntete)) return 0;

   for (int id = 0; id < 3; id++) {
         HeaderInfo header;
         header.id = blockEntete[id * sizeof(uint8_t) * 3];
         header.blockDeb = blockEntete[id * sizeof(uint8_t) * 3 + sizeof(uint8_t)];
         header.blockFin = blockEntete[id * sizeof(uint8_t) * 3 + sizeof(uint8_t) * 2];

         if (header.blockDeb != 0 || header.blockFin != 0) {
            headers.push_back(header);
         }

         if(deb > header.blockDeb) deb = header.blockDeb;
         if(fin < header.blockFin) fin = header.blockFin;

         ESP_LOGI(TAG.c_str(), "header.id: %d - header.blockDeb: %d - header.blockFin: %d", header.id, header.blockDeb, header.blockFin);
   }

   for(int x = 0; x < ((fin / 3) - (deb / 3) + 1); x++)
      if(!m_pNFC->MifareclassicReadDataSector(x + 1, dataSector[x][0], dataSector[x][1], dataSector[x][2])) return 0;
   
   auto fun = [this] (std::string& str, uint8_t blockDeb, uint8_t blockFin) {
         str.clear();

         ESP_LOGI(TAG.c_str(), "blockDeb: %d - blockFin: %d", blockDeb, blockFin);

         for (uint8_t block = blockDeb; block <= blockFin; block++) {
            int sectorIndex = (block - 1) / NBBLOCK_WRITEABLE - 1;
            int blockIdx = (block - 1) % (NBBLOCK_WRITEABLE);

            ESP_LOGI(TAG.c_str(), "Lecture - block: %d - Secteur: %d - block %d", block, sectorIndex, blockIdx);

            uint8_t data[BLOCK_SIZE];
            memcpy(data, dataSector[sectorIndex][blockIdx], BLOCK_SIZE);
            for(int i = 0; (i < BLOCK_SIZE) && (data[i] != 0xFF); i++)
               str += static_cast<char>(data[i]);
         }
   };

   // Lecture des données utilisateur à partir des secteurs
   for (const auto& header : headers) {
      switch(header.id) {
      case 1: 
         fun(this->m_user.handle, header.blockDeb, header.blockFin);
         break;
      case 2:
         fun(this->m_user.pseudoDiscord, header.blockDeb, header.blockFin);
         break;
      case 3:
         fun(this->m_user.email, header.blockDeb, header.blockFin);
         break;
      }
   }

   return 1;
}

void App::Run() {
   ESP_LOGI(TAG.c_str(), "Lancement de la boucle");
   ESP_LOGI(TAG.c_str(), "En attente d'une connexion bluetooth");
   // Séquence LED pour attente Bluetooth (initialisation avec clignotements irréguliers)
   int seq_bluetooth[] = {150, 250, 350, 200};


   GetSandGlass()->AddSequence(SandGlass::CreateSequence(4, seq_bluetooth), true);
   GetBluetooth()->waitForConnectionLoop();

   // Séquence LED pour attente Bluetooth (initialisation avec clignotements irréguliers)
   int seq[] = {250, 500, 250, 500};


   GetSandGlass()->AddSequence(SandGlass::CreateSequence(4, seq), true);
   auto err = [this]{
      GetBluetooth()->Send("1;3");
      ESP_LOGI(TAG.c_str(), "Retrait de la carte prematurée");
   };


   STATE_ESP state = IDLE, prev_state = IDLE_RETRAIT;
   for(;;) {
#ifndef CONFIG_PN532_WRITE

      if (state != prev_state) {
         prev_state = state;
         switch (state) {
            case IDLE:
               ESP_LOGI(TAG.c_str(), "En attente de la carte");
               break;
            
            case IDLE_RETRAIT:
               ESP_LOGI(TAG.c_str(), "En attente de retrait de la carte");
            break;

            default:
               break;
         }
      
      }

      switch (state) {
         case IDLE:
            GetGPIO()->SetLevel(LED_RED, true);
            GetGPIO()->SetLevel(LED_GREEN1, false);
            GetGPIO()->SetLevel(LED_GREEN2, false);
            GetBluetooth()->Send("1;0");
            if(m_pNFC->ReadId(50)) state = CARD_DETECTED; 
            break;
            
         case CARD_DETECTED:
            if(!m_pNFC->ReadId(50) ) { err(); continue; } 
            ESP_LOGI(TAG.c_str(), "Carte trouvée");
            ESP_LOGI(TAG.c_str(), "UID Length: %d bytes", m_pNFC->GetUidLength());
            ESP_LOGI(TAG.c_str(), "UID Value:");
            esp_log_buffer_hexdump_internal(TAG.c_str(), m_pNFC->GetUid(), m_pNFC->GetUidLength(), ESP_LOG_INFO);   

            GetGPIO()->SetLevel(LED_GREEN1, true);
            GetBluetooth()->Send("1;1");
            ESP_LOGI(TAG.c_str(), "Lecture des données stockée dans la carte");
            if(!RetrieveInformationFromNFC()) { err(); continue; }
            GetBluetooth()->Send("1;2");
            ESP_LOGI(TAG.c_str(), "Chargement terminé");
            while(GetGPIO()->IsHigh(BTN_POUS)) {
               if (!m_pNFC->ReadId(500)) err();
               vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            
            GetGPIO()->SetLevel(LED_RED, false);
            GetGPIO()->SetLevel(LED_GREEN2, true);
            GetBluetooth()->Send("1;4");
            ESP_LOGI(TAG.c_str(), "Accès autorisée");
            state = IDLE_RETRAIT;
            break;
         case IDLE_RETRAIT:
               GetBluetooth()->Send("1;5");
               if(!m_pNFC->ReadId(50) && reset) state = IDLE;
            break;
         default:
            break;
         }
#else
      while(!saveInfo) vTaskDelay(25 / portTICK_PERIOD_MS);
      saveInfo = false;

      ESP_LOGI(TAG.c_str(), "En attente de la carte");
      GetBluetooth()->Send("En attente de la carte");
      while(!m_pNFC->ReadId(0));
      
      ESP_LOGI(TAG.c_str(), "Ecriture de la carte en cours");
      GetBluetooth()->Send("Ecriture de la carte en cours");
      if(SaveInformationIntoNFC()) {
         ESP_LOGI(TAG.c_str(), "Information Enregistrée avec Succès");
         GetBluetooth()->Send("1");
      } else {
         ESP_LOGE(TAG.c_str(), "Erreur lors de l'enregistrement");
         GetBluetooth()->Send("0");
      }

      
#endif
   }
}

void App::TraitBluetooth(void* pObj, const uint8_t* data, uint16_t len){

   App* pApp = static_cast<App*>(pObj);
   ESP_LOGI(TAG.c_str(), "Message Recu: %.*s", len, data);
#ifndef CONFIG_PN532_WRITE
   pApp->reset = (data[0] == '4');
#else

   char *token;
   
   token = strtok(buf, ";");

   for(int x = 1; token != NULL; x++) {
      
      switch(x) {
      case 1:
         pApp->m_user.handle = string(token);
         ESP_LOGI(TAG.c_str(), "%s", pApp->m_user.handle.c_str());
         break;
      case 2:
         pApp->m_user.pseudoDiscord = string(token);
         ESP_LOGI(TAG.c_str(), "%s", pApp->m_user.pseudoDiscord.c_str());
         break;
      case 3:
         pApp->m_user.email = string(token);
         ESP_LOGI(TAG.c_str(), "%s", pApp->m_user.email.c_str());
         pApp->saveInfo = true;
         break;
      }

      token = strtok(NULL, ";");
   }   

   
#endif
}