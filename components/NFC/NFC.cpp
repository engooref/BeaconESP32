#include <stdio.h>
#include "NFC.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_log_internal.h>

#include "utils.h"

// #define PN532_DEBUG_EN
// #define MIFARE_DEBUG_EN

#ifdef PN532_DEBUG_EN
#define PN532_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PN532_DEBUG(fmt, ...)
#endif

#ifdef MIFARE_DEBUG_EN
#define MIFARE_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define MIFARE_DEBUG(fmt, ...)
#endif

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define PN532_DELAY(ms) vTaskDelay(ms / portTICK_RATE_MS)

using namespace std;

const std::string NFC::TAG = "NFC";

NFC::NFC() :
    _clk(gpio_num_t(CONFIG_PN532_SCK)),
    _miso(gpio_num_t(CONFIG_PN532_MISO)),
    _mosi(gpio_num_t(CONFIG_PN532_MOSI)),
    _ss(gpio_num_t(CONFIG_PN532_SS))
{
    gpio_pad_select_gpio(_clk);
    gpio_pad_select_gpio(_miso);
    gpio_pad_select_gpio(_mosi);
    gpio_pad_select_gpio(_ss);

    gpio_set_direction(_ss, GPIO_MODE_OUTPUT);
    gpio_set_level(_ss, 1);
    gpio_set_direction(_clk, GPIO_MODE_OUTPUT);
    gpio_set_direction(_mosi, GPIO_MODE_OUTPUT);
    gpio_set_direction(_miso, GPIO_MODE_INPUT);

    gpio_set_level(_ss, 0);

    PN532_DELAY(1000);

    // not exactly sure why but we have to send a dummy command to get synced up
    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
    SendCommandCheckAck(pn532_packetbuffer, 1, 1000);

    // ignore response!
    gpio_set_level(_ss, 1);
}


/**************************************************************************/
/*!
    @brief  Checks the firmware version of the PN5xx chip

    @returns  The chip's firmware version and ID
*/
/**************************************************************************/
uint32_t NFC::GetFirmwareVersion()
{
    uint32_t response;

    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

    if (!SendCommandCheckAck(pn532_packetbuffer, 1, 1000))
        return 0;

    // read data packet
    ReadData(pn532_packetbuffer, 12);

    // check some basic stuff
    if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6))
    {
        PN532_DEBUG("Firmware doesn't match!\n");
        return 0;
    }

    int offset = 6;
    response = pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];

    return response;
}


/**************************************************************************/
/*!
    @brief  Sends a command and waits a specified period for the ACK

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    The size of the command in bytes
    @param  timeout   timeout before giving up

    @returns  1 if everything is OK, 0 if timeout occured before an
              ACK was recieved
*/
/**************************************************************************/
// default timeout of one second
bool NFC::SendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout)
{
    // write the command
    WriteCommand(cmd, cmdlen);

    // Wait for chip to say its ready!
    if (!WaitReady(timeout))
        return false;


    // read acknowledgement
    if (!ReadAck())
    {
        PN532_DEBUG("No ACK frame received!\n");
        return false;
    }

    // For SPI only wait for the chip to be ready again.
    // This is unnecessary with I2C.
    if (!WaitReady(timeout))
        return false;


    return true; // ack'd command
}

/**************************************************************************/
/*!
    Writes an 8-bit value that sets the state of the PN532's GPIO pins

    @warning This function is provided exclusively for board testing and
             is dangerous since it will throw an error if any pin other
             than the ones marked "Can be used as GPIO" are modified!  All
             pins that can not be used as GPIO should ALWAYS be left high
             (value = 1) or the system will become unstable and a HW reset
             will be required to recover the PN532.

             pinState[0]  = P30     Can be used as GPIO
             pinState[1]  = P31     Can be used as GPIO
             pinState[2]  = P32     *** RESERVED (Must be 1!) ***
             pinState[3]  = P33     Can be used as GPIO
             pinState[4]  = P34     *** RESERVED (Must be 1!) ***
             pinState[5]  = P35     Can be used as GPIO

    @returns 1 if everything executed properly, 0 for an error
*/
void NFC::SetKey(uint8_t keyNumber, uint8_t *key)
{ 
    _keyNumber = keyNumber; 
    for(int i = 0; i < 6; i++) 
        _key[i] = key[i]; 
}

/**************************************************************************/
bool NFC::WriteGPIO(uint8_t pinstate)
{

    // Make sure pinstate does not try to toggle P32 or P34
    pinstate |= (1 << PN532_GPIO_P32) | (1 << PN532_GPIO_P34);

    // Fill command buffer
    pn532_packetbuffer[0] = PN532_COMMAND_WRITEGPIO;
    pn532_packetbuffer[1] = PN532_GPIO_VALIDATIONBIT | pinstate; // P3 Pins
    pn532_packetbuffer[2] = 0x00;                                // P7 GPIO Pins (not used ... taken by SPI)

    PN532_DEBUG("Writing P3 GPIO: %02x\n", pn532_packetbuffer[1]);

    // Send the WRITEGPIO command (0x0E)
    if (!SendCommandCheckAck(pn532_packetbuffer, 3, 1000))
        return 0x0;

    // Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0F) DATACHECKSUM 00)
    ReadData(pn532_packetbuffer, 8);

    PN532_DEBUG("Received:");
    for (int i = 0; i < 8; i++)
        PN532_DEBUG(" %02x", pn532_packetbuffer[i]);

    PN532_DEBUG("\n");

    return (pn532_packetbuffer[5] == 0x0F);
}

/**************************************************************************/
/*!
    Reads the state of the PN532's GPIO pins

    @returns An 8-bit value containing the pin state where:

             pinState[0]  = P30
             pinState[1]  = P31
             pinState[2]  = P32
             pinState[3]  = P33
             pinState[4]  = P34
             pinState[5]  = P35
*/
/**************************************************************************/
uint8_t NFC::ReadGPIO()
{
    pn532_packetbuffer[0] = PN532_COMMAND_READGPIO;

    // Send the READGPIO command (0x0C)
    if (!SendCommandCheckAck(pn532_packetbuffer, 1, 1000))
        return 0x0;

    // Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0D) P3 P7 IO1 DATACHECKSUM 00)
    ReadData(pn532_packetbuffer, 11);

    /* READGPIO response should be in the following format:

    uint8_t            Description
    -------------   ------------------------------------------
    b0..5           Frame header and preamble (with I2C there is an extra 0x00)
    b6              P3 GPIO Pins
    b7              P7 GPIO Pins (not used ... taken by SPI)
    b8              Interface Mode Pins (not used ... bus select pins)
    b9..10          checksum */

    int p3offset = 6;

    PN532_DEBUG("Received:");
    for (int i = 0; i < 11; i++)
        PN532_DEBUG(" %02x", pn532_packetbuffer[i]);

    PN532_DEBUG("\n");

    PN532_DEBUG("P3 GPIO: %02x\n", pn532_packetbuffer[p3offset]);
    PN532_DEBUG("P7 GPIO: %02x\n", pn532_packetbuffer[p3offset + 1]);
    PN532_DEBUG("IO GPIO: %02x\n", pn532_packetbuffer[p3offset + 2]);
    // Note: You can use the IO GPIO value to detect the serial bus being used
    switch (pn532_packetbuffer[p3offset + 2])
    {
    case 0x00: // Using UART
        PN532_DEBUG("Using UART (IO = 0x00)\n");
        break;
    case 0x01: // Using I2C
        PN532_DEBUG("Using I2C (IO = 0x01)\n");
        break;
    case 0x02: // Using SPI
        PN532_DEBUG("Using SPI (IO = 0x02)\n");
        break;
    }

    return pn532_packetbuffer[p3offset];
}

/**************************************************************************/
/*!
    @brief  Configures the SAM (Secure Access Module)
*/
/**************************************************************************/
bool NFC::SAMConfig()
{
    pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!

    if (!SendCommandCheckAck(pn532_packetbuffer, 4, 1000))
        return false;

    // read data packet
    ReadData(pn532_packetbuffer, 8);

    int offset = 5;
    return (pn532_packetbuffer[offset] == 0x15);
}

/**************************************************************************/
/*!
    Waits for an ISO14443A target to enter the field

    @param  cardBaudRate  Baud rate of the card
    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 bytes)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool NFC::ReadId(uint16_t timeout)
{
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = PN532_MIFARE_ISO14443A;

    if (!SendCommandCheckAck(pn532_packetbuffer, 3, timeout))
    {
        PN532_DEBUG("No card(s) read\n");
        return 0x0; // no cards read
    }

    // read data packet
    ReadData(pn532_packetbuffer, 20);
    // check some basic stuff

    /* ISO14443A card response should be in the following format:

    uint8_t            Description
    -------------   ------------------------------------------
    b0..6           Frame header and preamble
    b7              Tags Found
    b8              Tag Number (only one used in this example)
    b9..10          SENS_RES
    b11             SEL_RES
    b12             NFCID Length
    b13..NFCIDLen   NFCID                                      */

    PN532_DEBUG("Found %d tags\n", pn532_packetbuffer[7]);
    if (pn532_packetbuffer[7] != 1)
        return 0;

    uint16_t sens_res = pn532_packetbuffer[9];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[10];

    PN532_DEBUG("ATQA: %02x\n", sens_res);
    PN532_DEBUG("SAK: %02x\n", pn532_packetbuffer[11]);

    /* Card appears to be Mifare Classic */
    _uidLen = pn532_packetbuffer[12];

    PN532_DEBUG("UID:");
    for (uint8_t i = 0; i < pn532_packetbuffer[12]; i++) {
        _uid[i] = pn532_packetbuffer[13 + i];
        PN532_DEBUG(" %02x", _uid[i]);
    }

    PN532_DEBUG("\n");

    return 1;
}

/***** Mifare Classic Functions ******/

/**************************************************************************/
/*!
      Indicates whether the specified block number is the first block
      in the sector (block 0 relative to the current sector)
*/
/**************************************************************************/
bool NFC::MifareclassicIsFirstBlock(uint32_t uiBlock)
{
    return (uiBlock < 128) ? ((uiBlock) % 4 == 0) : ((uiBlock) % 16 == 0);
}

/**************************************************************************/
/*!
      Indicates whether the specified block number is the sector trailer
*/
/**************************************************************************/
bool NFC::MifareclassicIsTrailerBlock(uint32_t uiBlock)
{
    return (uiBlock < 128) ? ((uiBlock + 1) % 4 == 0) : ((uiBlock + 1) % 16 == 0);
}

/**************************************************************************/
/*!
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t NFC::MifareclassicAuthenticateBlock(uint32_t blockNumber)
{
    MIFARE_DEBUG("Trying to authenticate card\n");
    for (int i = 0; i < _uidLen; i++)
        MIFARE_DEBUG(" %02x", _uid[i]);
    
    MIFARE_DEBUG("\n");
    MIFARE_DEBUG("Using authentication KEY %c\n", _keyNumber ? 'B' : 'A');
    for (int i = 0; i < 6; i++)
        MIFARE_DEBUG(" %02x", _key[i]);
    
    MIFARE_DEBUG("\n");

    // Prepare the authentication command //
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE; /* Data Exchange Header */
    pn532_packetbuffer[1] = 1;                            /* Max card numbers */
    pn532_packetbuffer[2] = (_keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
    pn532_packetbuffer[3] = blockNumber; /* Block Number (1K = 0..63, 4K = 0..255 */
    memcpy(pn532_packetbuffer + 4, _key, 6);
    for (int i = 0; i < _uidLen; i++)
        pn532_packetbuffer[10 + i] = _uid[i]; /* 4 uint8_t card ID */
    
    if (!SendCommandCheckAck(pn532_packetbuffer, 10 + _uidLen, 1000))
        return 0;

    // Read the response packet
    ReadData(pn532_packetbuffer, 12);

    // check if the response is valid and we are authenticated???
    // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
    // Mifare auth error is technically uint8_t 7: 0x14 but anything other and 0x00 is not good
    if (pn532_packetbuffer[6] != 0x00)
    {
        MIFARE_DEBUG("Authentification failed\n");
        for (int i = 0; i < 12; i++)
            MIFARE_DEBUG(" %02x", pn532_packetbuffer[i]);
        MIFARE_DEBUG("\n");
        return 0;
    }

    return 1;
}

/**************************************************************************/
/*!
    Tries to read an entire 16-uint8_t data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the uint8_t array that will hold the
                          retrieved data (if any)

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t NFC::MifareclassicReadDataBlock(uint8_t blockNumber, uint8_t *data)
{
    MIFARE_DEBUG("Trying to read 16 bytes from block %d\n", blockNumber);

    if (blockNumber < lastblockRead - (lastblockRead % 4) || blockNumber >= lastblockRead + (4 - (lastblockRead % 4))) {
        if (!ReadId(1000)) return 0;
        if (!MifareclassicAuthenticateBlock(blockNumber)) return 0;
        lastblockRead = blockNumber;
    }   
    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;               /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = blockNumber;     /* Block Number (0..63 for 1K, 0..255 for 4K) */

    /* Send the command */
    if (!SendCommandCheckAck(pn532_packetbuffer, 4, 1000))
    {
        MIFARE_DEBUG("Failed to receive ACK for read command\n");
        return 0;
    }

    /* Read the response packet */
    ReadData(pn532_packetbuffer, 26);

    /* If uint8_t 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[6] != 0x00)
    {
        MIFARE_DEBUG("Unexpected response:");
        for (int i = 0; i < 26; i++)
            MIFARE_DEBUG(" %02x", pn532_packetbuffer[i]);
        MIFARE_DEBUG("\n");
        return 0;
    }

    /* Copy the 16 data bytes to the output buffer        */
    /* Block content starts at uint8_t 9 of a valid response */
    memcpy(data, pn532_packetbuffer + 7, BLOCK_SIZE);

    /* Display data for debug if requested */
    MIFARE_DEBUG("Block %d\n", blockNumber);
    for (int i = 0; i < BLOCK_SIZE; i++)
        MIFARE_DEBUG(" %02x", data[i]);
    MIFARE_DEBUG("\n");

    return 1;
}

uint8_t NFC::MifareclassicReadDataSector(uint8_t sectorNumber, uint8_t *block1, uint8_t *block2, uint8_t *block3)
{
    ESP_LOGI(TAG.c_str(), "Lecture du secteur %d sur la carte NFC", sectorNumber);  

    if (!MifareclassicReadDataBlock(sectorNumber * 4, block1)) return 0;
    if (!MifareclassicReadDataBlock(sectorNumber * 4 + 1, block2)) return 0;
    if (!MifareclassicReadDataBlock(sectorNumber * 4 + 2, block3)) return 0;

    return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 16-uint8_t data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          The uint8_t array that contains the data to write.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t NFC::MifareclassicWriteDataBlock(uint8_t blockNumber, uint8_t *data)
{
 
    MIFARE_DEBUG("Trying to write 16 bytes to block %d\n", blockNumber);

    if (blockNumber < lastblockRead - (lastblockRead % 4) || blockNumber >= lastblockRead + (4 - (lastblockRead % 4))) {
        if (!ReadId(1000)) return 0;
        if (!MifareclassicAuthenticateBlock(blockNumber)) return 0;
        lastblockRead = blockNumber;
    }   

    /* Prepare the first command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
    pn532_packetbuffer[3] = blockNumber;      /* Block Number (0..63 for 1K, 0..255 for 4K) */
    memcpy(pn532_packetbuffer + 4, data, BLOCK_SIZE); /* Data Payload */

    /* Send the command */
    if (!SendCommandCheckAck(pn532_packetbuffer, 20, 1000))
    {
        MIFARE_DEBUG("Failed to receive ACK for write command\n");
        return 0;
    }
    PN532_DELAY(10);

    /* Read the response packet */
    ReadData(pn532_packetbuffer, 26);

    return 1;
}
uint8_t NFC::MifareclassicWriteDataSector(uint8_t sectorNumber, uint8_t* block1, uint8_t* block2, uint8_t* block3)
{

    ESP_LOGI(TAG.c_str(), "Visualisation des données à écrire:");
    esp_log_buffer_hexdump_internal(TAG.c_str(), block1, BLOCK_SIZE, ESP_LOG_INFO);
    esp_log_buffer_hexdump_internal(TAG.c_str(), block2, BLOCK_SIZE, ESP_LOG_INFO); 
    esp_log_buffer_hexdump_internal(TAG.c_str(), block3, BLOCK_SIZE, ESP_LOG_INFO);

    ESP_LOGI(TAG.c_str(), "Enregistrement du secteur %d sur la carte NFC", sectorNumber);  

    if (!MifareclassicWriteDataBlock(sectorNumber * 4, block1)) return 0;
    if (!MifareclassicWriteDataBlock(sectorNumber * 4 + 1, block2)) return 0;
    if (!MifareclassicWriteDataBlock(sectorNumber * 4 + 2, block3)) return 0;

    return 1;
}

/**************************************************************************/
/*!
    @brief  set the PN532 as iso14443a Target behaving as a SmartCard
    @param  None
    #author Salvador Mendoza(salmg.net) new functions:
    -AsTarget
    -getDataTarget
    -setDataTarget
*/
/**************************************************************************/
uint8_t NFC::AsTarget()
{
    pn532_packetbuffer[0] = 0x8C;
    uint8_t target[] = {
        0x8C,             // INIT AS TARGET
        0x00,             // MODE -> BITFIELD
        0x08, 0x00,       //SENS_RES - MIFARE PARAMS
        0xdc, 0x44, 0x20, //NFCID1T
        0x60,             //SEL_RES
        0x01, 0xfe,       //NFCID2T MUST START WITH 01fe - FELICA PARAMS - POL_RES
        0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,                                    //PAD
        0xff, 0xff,                                                                        //SYSTEM CODE
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01, 0x00,            //NFCID3t MAX 47 BYTES ATR_RES
        0x0d, 0x52, 0x46, 0x49, 0x44, 0x49, 0x4f, 0x74, 0x20, 0x50, 0x4e, 0x35, 0x33, 0x32 //HISTORICAL BYTES
    };
    if (!SendCommandCheckAck(target, sizeof(target), 1000))
        return false;

    // read data packet
    ReadData(pn532_packetbuffer, 8);

    return (pn532_packetbuffer[5] == 0x15);
}
/**************************************************************************/
/*!
    @brief  retrieve response from the emulation mode

    @param  cmd    = data
    @param  cmdlen = data length
*/
/**************************************************************************/
uint8_t NFC::GetDataTarget(uint8_t *cmd, uint8_t *cmdlen)
{
    uint8_t length;
    pn532_packetbuffer[0] = 0x86;
    if (!SendCommandCheckAck(pn532_packetbuffer, 1, 1000))
    {
        PN532_DEBUG("Error en ack\n");
        return false;
    }

    // read data packet
    ReadData(pn532_packetbuffer, 64);
    length = pn532_packetbuffer[3] - 3;

    for (int i = 0; i < length; ++i)
        cmd[i] = pn532_packetbuffer[8 + i];

    *cmdlen = length;
    return true;
}

/**************************************************************************/
/*!
    @brief  set data in PN532 in the emulation mode

    @param  cmd    = data
    @param  cmdlen = data length
*/
/**************************************************************************/
uint8_t NFC::SetDataTarget(uint8_t *cmd, uint8_t cmdlen)
{
    uint8_t length;

    if (!SendCommandCheckAck(cmd, cmdlen, 1000))
        return false;

    // read data packet
    ReadData(pn532_packetbuffer, 8);
    
    length = pn532_packetbuffer[3] - 3;
    for (int i = 0; i < length; ++i)
        cmd[i] = pn532_packetbuffer[8 + i];
    
    cmdlen = length;

    return (pn532_packetbuffer[5] == 0x15);
}

/************** high level communication functions (handles both I2C and SPI) */

/**************************************************************************/
/*!
    @brief  Tries to read the SPI or I2C ACK signal
*/
/**************************************************************************/
bool NFC::ReadAck()
{
    uint8_t ackbuff[6];

    ReadData(ackbuff, 6);

    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/**************************************************************************/
/*!
    @brief  Return true if the PN532 is ready with a response.
*/
/**************************************************************************/
bool NFC::IsReady()
{
    gpio_set_level(_ss, 0);
    PN532_DELAY(10);
    SpiWrite(PN532_SPI_STATREAD);

    uint8_t x = SpiRead();

    gpio_set_level(_ss, 1);

    // Check if status is ready.
    return x == PN532_SPI_READY;
}

/**************************************************************************/
/*!
    @brief  Waits until the PN532 is ready.

    @param  timeout   Timeout before giving up
*/
/**************************************************************************/
bool NFC::WaitReady(uint16_t timeout)
{
    uint16_t timer = 0;
    while (!IsReady())
    {
        if (timeout != 0)
        {
            timer += 10;
            if (timer > timeout)
            {
                PN532_DEBUG("TIMEOUT!\n");
                return false;
            }
        }
        PN532_DELAY(10);
    }
    return true;
}

/**************************************************************************/
/*!
    @brief  Writes a command to the PN532, automatically inserting the
            preamble and required frame details (checksum, len, etc.)

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    Command length in bytes
*/
/**************************************************************************/
void NFC::WriteCommand(uint8_t *cmd, uint8_t cmdlen)
{
    uint8_t checksum;

    cmdlen++;

    PN532_DEBUG("Sending:");

    gpio_set_level(_ss, 0);
    PN532_DELAY(10); // or whatever the PN532_DELAY is for waking up the board
    SpiWrite(PN532_SPI_DATAWRITE);

    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    SpiWrite(PN532_PREAMBLE);
    SpiWrite(PN532_PREAMBLE);
    SpiWrite(PN532_STARTCODE2);

    SpiWrite(cmdlen);
    SpiWrite(~cmdlen + 1);

    SpiWrite(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

    PN532_DEBUG(" %02x %02x %02x %02x %02x %02x", (uint8_t)PN532_PREAMBLE, (uint8_t)PN532_PREAMBLE, (uint8_t)PN532_STARTCODE2, (uint8_t)cmdlen, (uint8_t)(~cmdlen + 1), (uint8_t)PN532_HOSTTOPN532);

    for (uint8_t i = 0; i < cmdlen - 1; i++)
    {
        SpiWrite(cmd[i]);
        checksum += cmd[i];
        PN532_DEBUG(" %02x", cmd[i]);
    }

    SpiWrite(~checksum);
    SpiWrite(PN532_POSTAMBLE);
    gpio_set_level(_ss, 1);

    PN532_DEBUG(" %02x %02x\n", (uint8_t)~checksum, (uint8_t)PN532_POSTAMBLE);
}

/**************************************************************************/
/*!
    @brief  Reads n bytes of data from the PN532 via SPI or I2C.

    @param  buff      Pointer to the buffer where data will be written
    @param  n         Number of bytes to be read
*/
/**************************************************************************/
void NFC::ReadData(uint8_t *buff, uint8_t n)
{
    gpio_set_level(_ss, 0);
    PN532_DELAY(10);
    SpiWrite(PN532_SPI_DATAREAD);

    PN532_DEBUG("Reading:");
    for (uint8_t i = 0; i < n; i++)
    {
        PN532_DELAY(10);
        buff[i] = SpiRead();
        PN532_DEBUG(" %02x", buff[i]);
    }
    PN532_DEBUG("\n");

    gpio_set_level(_ss, 1);    
}

/**************************************************************************/
/*!
    @brief  Low-level SPI write wrapper

    @param  c       8-bit command to write to the SPI bus
*/
/**************************************************************************/
void NFC::SpiWrite(uint8_t c) {
    gpio_set_level(_clk, 1);

    for (int8_t i = 0; i < 8; i++)
    {
        gpio_set_level(_clk, 0);
        gpio_set_level(_mosi, (c & _BV(i)));
        gpio_set_level(_clk, 1);
    }
}

/**************************************************************************/
/*!
    @brief  Low-level SPI read wrapper

    @returns The 8-bit value that was read from the SPI bus
*/
/**************************************************************************/
uint8_t NFC::SpiRead()
{
    int8_t x = 0;

    gpio_set_level(_clk, 1);

    for (int8_t i = 0; i < 8; i++)
    {
        if (gpio_get_level(_miso))
        {
            x |= _BV(i);
        }
        gpio_set_level(_clk, 0);
        gpio_set_level(_clk, 1);
    }

    return x;
}
