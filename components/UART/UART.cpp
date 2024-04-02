#include "UART.h"

#include "esp_log.h"

UART::UART() :
#ifdef CONFIG_UART_PORT_0
    m_port(UART_NUM_0),
#elif CONFIG_UART_PORT_1
    m_port(UART_NUM_1),
#elif CONFIG_UART_PORT_2
    m_port(UART_NUM_2),
#endif
    m_baudrate(CONFIG_UART_BAUDRATE),
    m_bufLen((CONFIG_UART_TX_BUF_SIZE > CONFIG_UART_RX_BUF_SIZE) ? CONFIG_UART_TX_BUF_SIZE : CONFIG_UART_RX_BUF_SIZE),
    m_buf(nullptr)

{
    ESP_LOGI(CONFIG_TAG_UART, "Initialisation classe UART");

    m_config.baud_rate = m_baudrate;
    m_config.data_bits = UART_DATA_8_BITS;
    m_config.parity = UART_PARITY_DISABLE;
    m_config.stop_bits = UART_STOP_BITS_1;
    m_config.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    m_config.rx_flow_ctrl_thresh = 122;

    ESP_ERROR_CHECK(uart_driver_install(m_port, CONFIG_UART_RX_BUF_SIZE, CONFIG_UART_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(m_port, &m_config));
    ESP_ERROR_CHECK(uart_set_pin(m_port, CONFIG_UART_PIN_TX, CONFIG_UART_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    m_buf = (uint8_t*)malloc(m_bufLen * sizeof(uint8_t));
}

UART::~UART(){
    ESP_LOGI(CONFIG_TAG_UART, "Destruction classe UART");    
    if(m_buf != nullptr) { free(m_buf); m_buf = nullptr; };

}

void UART::Read() {
    int len = uart_read_bytes(m_port, m_buf, (m_bufLen - 1), 20 / portTICK_PERIOD_MS);
    ESP_LOGI(CONFIG_TAG_UART, "Nb recv carac: %d", len);
    if (len > 0) {
        m_buf[len] = '\0';
        ESP_LOGI(CONFIG_TAG_UART, "%s", m_buf);
    }
}