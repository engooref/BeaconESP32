#ifndef CSOCKET_H_ 
#define CSOCKET_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "WiFi.h"
#include "Sandglass.h"

#include <iostream>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

typedef void (*t_ptfC)(void*, char*);

class Socket {
private:
    
    int m_sock;
    int m_cSock;

    sockaddr_in m_addr;

    TaskHandle_t m_taskHandle;
    
    bool m_isConnected;
    
    int m_indexSequence;

    t_ptfC m_pTraitFunc;
    void* m_pObj;

    WiFi* m_pWiFi;

public:
    Socket(SandGlass &sandGlass);
    ~Socket();

    void Send(std::string buf);

    bool IsConnected() const { return m_isConnected; }

    void ChangeTraitFunc(t_ptfC funcTraitementRecv, void* pObj) {m_pTraitFunc = funcTraitementRecv; m_pObj = pObj;};
   
 
public:
    static void SocketTest(void* msg);

private:
    static void vSocketTrait(void*);

};
#endif //CSOCKET_H_