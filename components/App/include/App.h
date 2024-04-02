#ifndef APP_H_
#define APP_H_

#include "Kernel.h"

class App : public Kernel {
private:
    void Run(); 
public:
    App();
    virtual ~App();
    
    bool reset;
    
private:
    static void TraitSocket(void* pObj, char* buf);
    static std::string TAG;
};
#endif //APP_H_