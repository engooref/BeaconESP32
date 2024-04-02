#ifndef SANDGLASS_H_
#define SANDGLASS_H_

#include <vector>
#include <string>

#include "../../Gpio/include/Gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

struct s_sequence {
    int nbItem;
    int* array;
};

class SandGlass {
private:
    
    Gpio* m_pGpio; 
    
    int m_indexSequence;
    std::vector<s_sequence> m_sequence;

    TaskHandle_t m_taskHandle; 

    bool m_isReinit;
    bool m_isBlock;
public:
    SandGlass(Gpio* pGpio, s_sequence sequenceRun);
    ~SandGlass();

    int AddSequence(s_sequence sequence, bool changeSequence = false);
    
    

    void RemoveSequence(int index);

    void ChangeSequence(int index);

private:
    static void vSandGlassRun(void*);
    static gpio_num_t GetGpio() { return (gpio_num_t)CONFIG_PIN_LED; }
public:    
    static s_sequence CreateSequence(int nbItem, int array[]);

private:
    static const std::string TAG;
};
#endif //SANDGLASS_H_