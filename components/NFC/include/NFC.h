#ifndef NFC_H_
#define NFC_H_

#define UID_LEN 7 
#define PN532_PACKBUFFSIZ 64

#define NB_SECTOR 16 // Sector 0 + 15 Sectors
#define BLOCK_SIZE 16
#define NBBLOCK_WRITEABLE 3 //exclu la ligne pour la cle

#include <cstring>
#include <string>
#include "driver/gpio.h"


class NFC {
private:
    gpio_num_t _clk;
    gpio_num_t _miso;
    gpio_num_t _mosi;
    gpio_num_t _ss;

    uint8_t lastblockRead = 0xFF;

    uint8_t _uid[UID_LEN];       // ISO14443A uid
    uint8_t _uidLen = UID_LEN;       // uid len
    uint8_t _key[6];       // Mifare Classic key
    uint8_t _keyNumber;    // 0x00 Key A
    uint8_t _inListedTag;  // Tg number of inlisted tag.

    uint8_t pn532ack[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    uint8_t pn532response_firmwarevers[6] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};
    uint8_t pn532_packetbuffer[PN532_PACKBUFFSIZ];
public:
    NFC();

    uint32_t GetFirmwareVersion();
    bool SendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout);
    bool SAMConfig();

    bool    MifareclassicIsFirstBlock(uint32_t uiBlock);
    bool    MifareclassicIsTrailerBlock(uint32_t uiBlock);
    uint8_t MifareclassicAuthenticateBlock(uint32_t blockNumber);
    uint8_t MifareclassicReadDataBlock(uint8_t blockNumber, uint8_t *data);
    uint8_t MifareclassicReadDataSector(uint8_t sectorNumber, uint8_t* block1, uint8_t* block2, uint8_t* block3);
    uint8_t MifareclassicWriteDataBlock(uint8_t blockNumber, uint8_t *data);
    uint8_t MifareclassicWriteDataSector(uint8_t sectorNumber, uint8_t* block1, uint8_t* block2, uint8_t* block3);

    uint8_t AsTarget();
    uint8_t GetDataTarget(uint8_t *cmd, uint8_t *cmdlen);
    uint8_t SetDataTarget(uint8_t *cmd, uint8_t cmdlen);

    bool ReadId(uint16_t timeout);

    uint8_t GetUidLength() const { return _uidLen; };
    const uint8_t* GetUid() const { return _uid; };

    void SetKey(uint8_t keyNumber, uint8_t* key);

private:
    bool WriteGPIO(uint8_t pinstate);
    uint8_t ReadGPIO();

    void WriteCommand(uint8_t *cmd, uint8_t cmdlen);
    void ReadData(uint8_t *buff, uint8_t n);

    bool ReadAck();
    bool IsReady();
    bool WaitReady(uint16_t timeout);
    
    void SpiWrite(uint8_t c);
    uint8_t SpiRead();

private:
    static const std::string TAG;
};
#endif //NFC_H_