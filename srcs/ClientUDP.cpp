#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <fstream>
#include <vector>
#include <set>
#include <utility>
#include <functional>
#include <net/if.h>
#include <sys/ioctl.h>

#define PORT 9000
#define BUFFER_SIZE 8192
#define MESSAGE_SERVER "Hello from client."

std::vector<std::pair<std::string, std::pair<std::string, int>>> ipList;
std::vector<std::pair<std::string, int>> vizinhos;
std::string localIp;

void errorFunction(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

void sendMessage(int sockfd, std::string router, std::string &message)
{
    struct sockaddr_in routerAddr;
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(PORT);
    routerAddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t addrLen = sizeof(routerAddr);

    if(ipList.size() > 0) {
        for(int i = 0; i < ipList.size(); i++) {
            if(ipList[i].second.first == router) {
                routerAddr.sin_addr.s_addr = inet_addr(ipList[i].first.c_str());
                break;
            }
        }
    }

    int sendLen = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
    if (sendLen < 0){
        std::cerr << "Error sending message" << std::endl;
    }
}

void receiveMessage(int sockfd)
{
    char buffer[BUFFER_SIZE];
    struct sockaddr_in receiveAddr;
    receiveAddr.sin_family = AF_INET;
    receiveAddr.sin_port = htons(PORT);
    receiveAddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t addrLen = sizeof(receiveAddr);

    while (true)
    {
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&receiveAddr, &addrLen);
        if (recvLen < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else
            {
                std::cerr << "Error to receive the message" << std::endl;
            }
        }
        else
        {
            buffer[recvLen] = '\0';

            std::string message(buffer);
            //std::cout << "mensagem recebida foi: " << message << std::endl;
            if (message[0] == '@')
            {   
                //std::cout << "ENTROU NO @" << std::endl;
                //std::cout << message << std::endl;
                std::string routerList = message.substr(1); // remove 1st char
                std::string delimiter = "@";
                std::string ipReceive = std::string(inet_ntoa(receiveAddr.sin_addr));

                for(int i = 0; i < vizinhos.size(); i++){
                    if (ipReceive == vizinhos[i].first)
                    {
                        std::cout << "Resetando timer do ip: " << ipReceive << std::endl;
                        vizinhos[i].second = 35;
                    }
                }

                while (routerList.size() > 0)
                {
                    // separando o ip e a métrica
                    int posIP = routerList.find(delimiter);
                    if (posIP == -1) {
                        //chegou no ultimo IP
                        posIP = routerList.length();
                    }
                    
                    // <ip>-<metrica>
                    std::string router = routerList.substr(0, posIP);

                    // separando a métrica para fazer o incremento
                    int posMetric = router.find("-");

                    // ip e metrica separados
                    std::string ip = router.substr(0, posMetric);
                    int metric = std::stoi(router.substr(posMetric + 1));

                    // teste printando eles
                    //std::cout << "router:" << router << " -> " << ip << "-" << metric << "\n";

                    

                    // verificando se o ip já está na lista e caso não esteja, adicionando na lista e com a métrica maior
                    bool found = false;
                    if (ipList.size() > 0)
                    {
                        for (int i = 0; i < ipList.size(); i++)
                        {

                            if (ipList[i].second.first == ip)
                            {
                                found = true;
                                //std::cout << "encontrou igual ou o localIP" << std::endl;
                                if ((metric + 1) < ipList[i].second.second)
                                {
                                    ipList[i].second.first = metric;
                                    ipList[i].first = std::string(inet_ntoa(receiveAddr.sin_addr));
                                    //std::cout << "Metric updated: " << ip << std::endl;
                                }
                            }
                        }
                    }

                    if (!found) {
                        if(ip != localIp) {
                            ipList.push_back(std::make_pair(ipReceive, std::make_pair(ip, (metric + 1))));
                            std::cout << "New router added: " << ip << std::endl;
                        }
                    }

                    // apagando a parte usada da mensagem para ir para a proxima 
                    routerList.erase(0, posIP + delimiter.length());
                    continue; 
                }
            }
            else if (message[0] == '!')
            {
                //std::cout << "ENTROU NO !" << std::endl;
                std::string messageDelimiter = ";";
                std::string userMessage = message;

                std::string routerOrigem = userMessage.substr(0, userMessage.find(messageDelimiter));
                routerOrigem = routerOrigem.substr(1);

                std::string routerDestino = userMessage.substr(userMessage.find(messageDelimiter) + 1, userMessage.size());
                routerDestino = routerDestino.substr(0, routerDestino.find(messageDelimiter));

                //std::cout << "routerOrigem:" << routerOrigem << std::endl;
                //std::cout << "routerDestino:" << routerDestino << std::endl;
                
                if(routerDestino != localIp){
                    sendMessage(sockfd, routerDestino, message);
                } else{
                    std::cout << userMessage << std::endl;
                }
            }
            else if (message[0] == '*') {
                std::cout << "ENTROU NO *" << std::endl;
                // novo roteador se conectou, vizinho novo, tem q adicionar na lista com metrica 0, olha no enunciado do trabalho
                bool exists = false;
                std::string ipVizinho = message.substr(1);
                std::cout << ipVizinho << std::endl;
                for(int i = 0; i < ipList.size(); i++) {
                    if(ipList[i].second.first == ipVizinho) {
                        exists = true;
                        ipList[i].first = ipVizinho;
                        ipList[i].second.second = 1;
                        break;
                    }
                }
                if(exists == false) {
                    std::cout << "New router connected: " << ipVizinho << std::endl;
                    ipList.push_back(std::make_pair(ipVizinho, std::make_pair(ipVizinho, 1)));
                    vizinhos.push_back(std::make_pair(ipVizinho, 35));
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void sendIpList(int sockfd)
{
    struct sockaddr_in routerAddr;
    socklen_t addrLen = sizeof(routerAddr);
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(PORT);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        std::string message = "";
        for (int i = 0; i < ipList.size(); i++)
        {
            message += "@" + ipList[i].second.first + "-" + std::to_string(ipList[i].second.second);
        }

        if (vizinhos.size() > 0)
        {
            for (int i = 0; i < vizinhos.size(); i++)
            {
                routerAddr.sin_addr.s_addr = inet_addr(vizinhos[i].first.c_str());
                
                //std::cout << "IpList foi enviada para o ip: " << ipList[i].first << std::endl;
                int sendLen = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
                if (sendLen < 0)
                {
                    std::cerr << "Error sending the ip list" << std::endl;
                }
            }
        }
    }
}

void printIpList()
{
    while (true)
    {
        if(ipList.size() == 0){
            std::cout << "No routers connected" << std::endl;
        } else{
        std::cout << "IP List:" << std::endl;
        for (int i = 0; i < ipList.size(); i++)
        {
            std::cout << ipList[i].second.first << "-" << ipList[i].second.second << std::endl;
        }
        }

        std::this_thread::sleep_for(std::chrono::seconds(25));
    }
}

void countDisconnect() {
    while (true) {
        for (int i = 0; i < vizinhos.size(); i++) {
            if (vizinhos[i].second == 0) {
                std::cout << "Router " << vizinhos[i].first << " disconnected" << std::endl;
                ipList.erase(ipList.begin() + i);
                vizinhos.erase(vizinhos.begin() + i);
            }
            else if (vizinhos[i].second > 0)
                vizinhos[i].second--;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in routerAddr;
    socklen_t addrLen = sizeof(routerAddr);
    std::string messageUser;
    localIp = argv[1];
    // std::cout << "IP Address of user: " << localIp << std::endl;

    // socket();

    sockfd = (socket(AF_INET, SOCK_DGRAM, 0));
    if (sockfd < 0)
        errorFunction("Error to create the socket");
    
    // routerAddr();
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(PORT);
    routerAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&routerAddr, sizeof(routerAddr)) < 0) {
        close(sockfd);
        errorFunction("Error to bind the socket");
    }

    std::ifstream file("roteadores.txt");

    if (!file.is_open()) {
        std::cerr << "Error opening file: roteadores.txt" << std::endl;
        return -1;
    }
    else {
        std::cout << "Arquivo de configuracao lido - ";
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << "Adding router: " << line << std::endl;
        ipList.push_back(std::make_pair(line, std::make_pair(line, 1)));
        vizinhos.push_back(std::make_pair(line, 35));
        std::string msg = '*' + localIp;
        routerAddr.sin_addr.s_addr = inet_addr(line.c_str());
        int sendLen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
        std::cout << msg << std::endl;
    }

    file.close();

    std::thread printIpListThread(printIpList);
    printIpListThread.detach();

    std::thread countDisconnectThread(countDisconnect);
    countDisconnectThread.detach();

    // Start receiving messages
    std::thread receiveThread(receiveMessage, sockfd);
    receiveThread.detach();

    // Periodically send IP list
    std::thread sendIpListThread(sendIpList, sockfd);
    sendIpListThread.detach();

    while (true)
    {
        //std::cout << "Iplist size: " << ipList.size()<< std::endl;
        // std::cout << "Digite a mensagem: ";
        std::string messageSend = "!" + localIp + ";";
        getline(std::cin, messageUser);
        messageSend += messageUser;
        std::string messageAux = messageUser;
        messageAux = messageAux.substr(0, messageAux.find(";"));
        //std::cout << messageAux << std::endl;
        //std::cout << "Message to the other user: " << messageSend << std::endl;
        for (int i = 0; i < ipList.size(); i++)
        {
            if(ipList[i].second.first == messageAux){
                std::cout << "Enviando para: " << ipList[i].first << std::endl;
                routerAddr.sin_addr.s_addr = inet_addr(ipList[i].first.c_str());
                int sendLen = sendto(sockfd, messageSend.c_str(), messageSend.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
                if (sendLen < 0){
                    std::cerr << "Error sending message" << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
