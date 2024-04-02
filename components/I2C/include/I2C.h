#ifndef I2C_H_
#define I2C_H_

#include "driver/i2c.h"
#include <vector>

class I2C {
private:
    i2c_port_t m_i2cMasterPort;
    i2c_config_t m_i2cConf;
public:
    I2C();
    ~I2C();

    esp_err_t Read(uint8_t addr, uint8_t* pReg, size_t lenReg, uint8_t* dataBuf, size_t lenData);
    esp_err_t Write(uint8_t addr, uint8_t* pReg = nullptr, size_t lenReg = 0, uint8_t* pData = nullptr, size_t lenData = 0);

    void Scan(std::vector<uint8_t>& i2cVec);
};

#endif //I2C_H_