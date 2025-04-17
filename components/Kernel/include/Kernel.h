#ifndef KERNEL_H
#define KERNEL_H

#include "esp_log.h"

#include "Sandglass.h"

#ifdef CONFIG_I2C
#include "I2C.h"
#endif

#ifdef CONFIG_BLUETOOTH
#include "Bluetooth.h"
#endif

#ifdef CONFIG_UART
#include "UART.h"
#endif

class Kernel{
private:
    Gpio* m_pGpio;
    SandGlass* m_pSandGlass;

#ifdef CONFIG_I2C
    I2C* m_pI2C;
#endif

#ifdef CONFIG_UART
    UART* m_pUart;
#endif 

#ifdef CONFIG_BLUETOOTH
    BluetoothManager* m_pBluetooth;
#endif


public:
    Kernel();
    virtual ~Kernel();

protected:
#ifdef CONFIG_SPIFFS
    esp_err_t InitSpiffs();
#endif

    void Run();

    Gpio* GetGPIO() const { return m_pGpio; }
    SandGlass* GetSandGlass() const { return m_pSandGlass; }

#ifdef CONFIG_I2C
    I2C* GetI2C() const {return m_pI2C; }
#endif

#ifdef CONFIG_UART
    UART* GetUART() const { return m_pUart; }
#endif 

#ifdef CONFIG_BLUETOOTH
    BluetoothManager* GetBluetooth() const { return m_pBluetooth; }
#endif

private:
    static const std::string TAG;
};

#endif