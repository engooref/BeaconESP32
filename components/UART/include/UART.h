#ifndef UART_H_
#define UART_H_

#include "driver/uart.h"

class UART {
private:
    uart_port_t m_port;
    uart_config_t m_config;
    int m_baudrate;

    int m_bufLen;
    uint8_t* m_buf;

public:
    UART();
    ~UART();

    void Read();
};

#endif