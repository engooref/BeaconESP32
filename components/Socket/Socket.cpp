#include "Socket.h"

#include "esp_log.h"
#include "Kernel.h"

using namespace std;

Socket::Socket(SandGlass &sandGlass) : 
    m_sock(-1),
    m_cSock(-1),
    m_taskHandle(NULL),
    m_isConnected(false),
    m_pTraitFunc(nullptr),
    m_pObj(nullptr),
    m_pWiFi(nullptr)
{
    int seq[] = {10, 15, 10, 15};
    int indexSeq = sandGlass.AddSequence(SandGlass::CreateSequence(4, seq), true);
    
    string strErr;
    ESP_LOGI(CONFIG_TAG_SOCKET, "Creation de la classe Socket");

    m_pWiFi = new WiFi;

    
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(m_sock < 0){
        strErr = "Impossible de creer le socket: errno " + to_string(errno);
        throw runtime_error(strErr);
    }

    ESP_LOGI(CONFIG_TAG_SOCKET, "Socket creer");

    m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(CONFIG_SOCKET_PORT);

    if(bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr))){
        close(m_sock);
        strErr = "Impossible de lier le socket: errno " + to_string(errno);
        throw runtime_error(strErr);
    }

    if (listen(m_sock, 1)) {
        close(m_sock);
        strErr = "Erreur durant l'ecoute: errno " + to_string(errno);
        throw runtime_error(strErr);
    }

    char ipStr[16] = "";
    WiFi::GetIp(ipStr);
    ESP_LOGI(CONFIG_TAG_SOCKET, "Serveur demarrer. Ip: %s Port: %d", ipStr, CONFIG_SOCKET_PORT);
    
    BaseType_t xTaskFinish = xTaskCreate(
                                vSocketTrait, 
                                "Socket Traitement", 
                                4096, 
                                (void*)this, 
                                1, 
                                &m_taskHandle);

    if(xTaskFinish != pdPASS){
        throw runtime_error("Tâche Socket non creer");
    }

    ESP_LOGI(CONFIG_TAG_SOCKET, "Tâche Socket creer");
    sandGlass.RemoveSequence(indexSeq);
}

Socket::~Socket(){
    close(m_sock);
    if(m_taskHandle) { vTaskDelete(m_taskHandle); m_taskHandle = NULL;}
    if(m_pWiFi) {delete m_pWiFi; m_pWiFi = nullptr;}
}

void Socket::Send(string buf){
    if(m_isConnected && m_cSock != -1){
        ESP_LOGI(CONFIG_TAG_SOCKET, "Send: %s", buf.c_str());
        buf += '\n';
        send(m_cSock, buf.c_str(), buf.length(), 0);
    }
}

void Socket::vSocketTrait(void* pObjSock){

    Socket* pSocket = static_cast<Socket*>(pObjSock);
    sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int nbRecv = 0;

    char buf[CONFIG_SOCKET_BUF_SIZE];

    for(;;){
    
        ESP_LOGI(CONFIG_TAG_SOCKET, "En attente d'une connexion entrante");
        pSocket->m_isConnected = false;

        if((pSocket->m_cSock = accept(pSocket->m_sock, (sockaddr*)&source_addr, &addr_len)) < 0){
            ESP_LOGE(CONFIG_TAG_SOCKET, "Impossible d'accepter la connexion: errno %d", errno);
            close(pSocket->m_sock);
            continue;
        }

        
        char addr_str[128];
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(CONFIG_TAG_SOCKET, "Connexion établi, IP du client: %s", addr_str);
        
        pSocket->m_isConnected = true;
        for(;;){
            if((nbRecv = recv(pSocket->m_cSock, buf, CONFIG_SOCKET_BUF_SIZE - 1, 0)) <= 0){
                if(!nbRecv){
                    ESP_LOGI(CONFIG_TAG_SOCKET, "Fermeture du socket demandé par le client");
                } else {
                    ESP_LOGE(CONFIG_TAG_SOCKET, "Erreur dans la reception des données, fermeture du client");
                }
                send(pSocket->m_cSock, "", 0, 0); 
                close(pSocket->m_cSock);
                pSocket->m_cSock = -1;
                break;
            } else {
                buf[nbRecv] = '\0';
                if (pSocket->m_pTraitFunc != nullptr)
                    pSocket->m_pTraitFunc(pSocket->m_pObj, buf);      
            }
        }
    }

    send(pSocket->m_cSock, "", 0, 0); 
    close(pSocket->m_cSock);
    pSocket->m_cSock = -1;
}

void Socket::SocketTest(void* msg){
    ESP_LOGI(CONFIG_TAG_SOCKET, "%s", static_cast<const char*>((msg)));
}