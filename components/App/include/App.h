#ifndef APP_H_
#define APP_H_

#include "Kernel.h"
#include "NFC.h"


struct s_user {
    std::string handle;
    std::string pseudoDiscord;
    std::string email;

    std::string toString(std::string delimiter = "") {
        return handle + delimiter + pseudoDiscord + delimiter + email;
    }
};

struct HeaderInfo {
    uint8_t id;       // Identifiant de l'information
    uint8_t blockDeb; // Bloc de début de l'information
    uint8_t blockFin; // Bloc de fin de l'information
};

enum STATE_ESP {
    IDLE = 0, //En attente
    CARD_DETECTED, //Carte Trouve
    IDLE_RETRAIT, //En Attente de retrait
};
 
class App : public Kernel {
private:
    NFC* m_pNFC;

    s_user m_user;

    uint8_t blockEntete[BLOCK_SIZE] = {0};
    uint8_t dataSector[NB_SECTOR - 1][NBBLOCK_WRITEABLE][BLOCK_SIZE] = {0};

#ifndef CONFIG_PN532_WRITE    
    bool reset = false;
#else
    bool saveInfo = false;
#endif
    
private:
    void Run(); 
public:
    App();
    virtual ~App();
    
    uint8_t SaveInformationIntoNFC();
    uint8_t RetrieveInformationFromNFC();


private:
    static void TraitBluetooth(void* pObj, const uint8_t* data, uint16_t len);
    static std::string TAG;
};
#endif //APP_H_