#include <iostream>

#include "Sandglass.h"
#include "esp_log.h"


using namespace std;


const string SandGlass::TAG = "SandGlass";

SandGlass::SandGlass(Gpio* pGpio, s_sequence sequenceRun) :
    m_pGpio(pGpio),
    m_indexSequence(0),
    m_taskHandle(NULL),
    m_isReinit(false),
    m_isBlock(false)
{
    ESP_LOGI(TAG.c_str(), "Creation de la classe SandGlass");

    ESP_LOGI(TAG.c_str(), "Configuration de la LED");
    m_pGpio->ChangeMode(GPIO_MODE_INPUT_OUTPUT);
    m_pGpio->AddGPIO(GetGpio());
    m_pGpio->ApplySetting();

    AddSequence(sequenceRun, true);

    ESP_LOGI(TAG.c_str(), "Configuration terminé");
    ESP_LOGI(TAG.c_str(), "Lancement de la tâche en arrière plan");

    BaseType_t xTaskFinish = xTaskCreate(
                                    vSandGlassRun, 
                                    "SandGlass Run", 
                                    4096, 
                                    (void*)this, 
                                    1, 
                                    &m_taskHandle);

    if(xTaskFinish != pdPASS){
        throw runtime_error("Tâche SandGlass non créée");
    } else {
        ESP_LOGI(TAG.c_str(), "Lancement reussi");
    }

}

SandGlass::~SandGlass()
{
    ESP_LOGI(TAG.c_str(), "Destruction de la classe SandGlass");
    if(m_taskHandle) { vTaskDelete(m_taskHandle); m_taskHandle = NULL; }
}

void SandGlass::vSandGlassRun(void* pObjSand){
    ESP_LOGI(TAG.c_str(), "Début de la tache de fond");

    SandGlass* pSandGlass = static_cast<SandGlass*>(pObjSand);
    int nbSequence = pSandGlass->m_sequence[pSandGlass->m_indexSequence].nbItem;

    for (int i =0; i< pSandGlass->m_sequence[pSandGlass->m_indexSequence].nbItem; i++)
        ESP_LOGI(TAG.c_str(), "%d", pSandGlass->m_sequence[pSandGlass->m_indexSequence].array[i]);

    for(int x = 0;;x++) {
        if (pSandGlass->m_isBlock) continue;
        if (pSandGlass->m_isReinit){
            ESP_LOGI(TAG.c_str(), "Changement Séquence");

            pSandGlass->m_pGpio->SetLevel(GetGpio(), false);
            pSandGlass->m_isReinit = false;
            nbSequence = pSandGlass->m_sequence[pSandGlass->m_indexSequence].nbItem;
            x = 0;
        }

        pSandGlass->m_pGpio->SetLevel(GetGpio(), !pSandGlass->m_pGpio->IsHigh(GetGpio()));
        vTaskDelay(pdMS_TO_TICKS(pSandGlass->m_sequence[pSandGlass->m_indexSequence].array[x % nbSequence]));
    }

}

int SandGlass::AddSequence(s_sequence sequence, bool changeSequence){
    ESP_LOGI(TAG.c_str(), "Ajout Séquence n°%d", m_sequence.size() + 1);
    m_isBlock = true;

    m_sequence.push_back(sequence);
    if (changeSequence) { ChangeSequence(m_sequence.size() - 1); } 
    
    m_isBlock = false;
    return m_sequence.size() - 1;
}

void SandGlass::RemoveSequence(int index){
    if (index <= 0 || index >= m_sequence.size()) return;
    ESP_LOGI(TAG.c_str(), "Suppression Séquence n°%d", index);

    m_isBlock = true;
    if (m_indexSequence == index)
        ChangeSequence(0);

    
    free(m_sequence[index].array);
    m_sequence.erase(m_sequence.begin() + index);

    m_isBlock = false;
}

s_sequence SandGlass::CreateSequence(int nbItem, int array[]){
    s_sequence newSequence;

    newSequence.nbItem = nbItem;
    newSequence.array = (int*)malloc(nbItem * sizeof(int));

    for (int i = 0; i < nbItem; i++){
        newSequence.array[i] = array[i];
    }

    return newSequence;
}

void SandGlass::ChangeSequence(int index){
    if (index >= m_sequence.size()) return;
    m_isBlock = true;
    
    m_indexSequence = index;
    m_isReinit = true;

    m_isBlock = false;
};
