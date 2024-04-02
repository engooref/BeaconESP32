#ifndef GPIO_H_
#define GPIO_H_

#include "driver/gpio.h"

#include "esp_log.h"
#include "sdkconfig.h"
#include <string>

#define GPIO_MODE_DEFAULT GPIO_MODE_DISABLE
#define GET_GPIO(gpio_num) (1ULL<<gpio_num)

class Gpio {
private:
    gpio_config_t m_io_conf;

public:
    Gpio();
    ~Gpio();

    void AddGPIO(gpio_num_t gpio_num) {
        ESP_LOGI(TAG.c_str(), "Ajout de la GPIO %d", (int)gpio_num);
        m_io_conf.pin_bit_mask |= GET_GPIO(gpio_num);
    };

    void RemoveGPIO(gpio_num_t gpio_num) {
        ESP_LOGI(TAG.c_str(), "Retrait de la GPIO %d", (int)gpio_num);
        m_io_conf.pin_bit_mask ^= GET_GPIO(gpio_num);
    };

    void ChangeMode(gpio_mode_t mode) {
        ESP_LOGI(TAG.c_str(), "Changement de mode pour les GPIO");
        m_io_conf.mode = mode;
    };

    void ApplySetting() {
        ESP_LOGI(TAG.c_str(), "Application des parametres enregistrer");
        gpio_config(&m_io_conf);
        m_io_conf.pin_bit_mask = 0;
    };

    void SetLevel(gpio_num_t gpio_num, bool isHigh){
        gpio_set_level(gpio_num, (uint32_t)isHigh);
    };

    bool IsHigh(gpio_num_t gpio_num){
        return (bool)gpio_get_level(gpio_num);
    };

private:
    static std::string TAG;
};
#endif //GPIO_H_