#include "Gpio.h"

std::string Gpio::TAG = "GPIO";

Gpio::Gpio() :
    m_io_conf({})
{
    ESP_LOGI(TAG.c_str(), "Creation de la classe Gpio");

    m_io_conf.intr_type = GPIO_INTR_DISABLE;
    m_io_conf.mode = GPIO_MODE_DEFAULT;
    m_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    m_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
}

Gpio::~Gpio()
{
    ESP_LOGI(TAG.c_str(), "Destruction de la classe Gpio");
}



