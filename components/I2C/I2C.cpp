#include "I2C.h"

#include "esp_log.h"
#include <iostream>


#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */         
#define ACK_VAL         (i2c_ack_type_t)0x0              /*!< I2C ack value */
#define NACK_VAL        (i2c_ack_type_t)0x1              /*!< I2C nack value */

using namespace std;

I2C::I2C() :
#if CONFIG_I2C_PORT_0
    m_i2cMasterPort(I2C_NUM_0)
#elif CONFIG_I2C_PORT_1
    m_i2cMasterPort(I2C_NUM_1)
#endif
{
	ESP_LOGI(CONFIG_TAG_I2C, "Creation de la classe I2C");
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
    };
    conf.master.clk_speed = CONFIG_I2C_FREQ;

    m_i2cConf = conf;
    ESP_ERROR_CHECK(i2c_driver_install(m_i2cMasterPort, I2C_MODE_MASTER, I2C_MASTER_TX_BUF_DISABLE, I2C_MASTER_RX_BUF_DISABLE, 0));
    ESP_ERROR_CHECK(i2c_param_config(m_i2cMasterPort, &conf));
    ESP_LOGI(CONFIG_TAG_I2C, "Fin de configuration");
}

I2C::~I2C(){
	ESP_LOGI(CONFIG_TAG_I2C, "Desctruction de la classe I2C");
}

esp_err_t I2C::Read(uint8_t addr, uint8_t* pReg, size_t lenReg, uint8_t * dataBuf, size_t len){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    // first, send device address (indicating write) & register to be read
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, ( addr << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN));
    // send register we want
    for (int x = 0; x < lenReg; x++){
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, pReg[x], ACK_CHECK_EN));
    }
    // Send repeated start
    ESP_ERROR_CHECK(i2c_master_start(cmd));

    // now send device address (indicating read) & read data
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, ( addr << 1 ) | I2C_MASTER_READ, ACK_CHECK_EN));
    if (len > 1) {
        ESP_ERROR_CHECK(i2c_master_read(cmd, dataBuf, len - 1, ACK_VAL));
    }

    ESP_ERROR_CHECK(i2c_master_read_byte(cmd, dataBuf + len - 1, NACK_VAL));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_LOGI(CONFIG_TAG_I2C, "Envoi commande");
    esp_err_t ret = i2c_master_cmd_begin(m_i2cMasterPort, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGI(CONFIG_TAG_I2C, "Fin commande");

    i2c_cmd_link_delete(cmd);
	return ret;  
}

esp_err_t I2C::Write(uint8_t addr, uint8_t* pReg, size_t lenReg, uint8_t* pData, size_t lenData){
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN));
	if(pReg != nullptr){
        cout << "nb Reg: " << lenReg << endl;
        for (int x = 0; x < lenReg; x++) {
    		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, pReg[x], ACK_CHECK_EN));
        }
	}
	if(pData != nullptr){
        cout << "nb Data: " << lenData << endl;

		ESP_ERROR_CHECK(i2c_master_write(cmd, pData, lenData, ACK_CHECK_EN));
	}
    
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(m_i2cMasterPort, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	return ret;
}

void I2C::Scan(std::vector<uint8_t>& i2cVec){
	ESP_LOGI(CONFIG_TAG_I2C, "Scan");

	for (uint8_t i = 0x00; i < 0xB0; i++) {
		if(Write(i) == ESP_OK){
			i2cVec.push_back(i);
		}
	}
}