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
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Hello from client."
#define ROUTER_LIST_PREFIX "ROUTER_LIST:"
#define MSG_PREFIX "MSG:"

std::vector<std::pair<std::string, std::pair<std::string, int>>> ipList;
std::vector<std::pair<std::string, int>> vizinhos;
std::string localIp;

void errorFunction(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
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
            if (message.rfind(ROUTER_LIST_PREFIX, 0) == 0)
            {
                std::string routerList = message.substr(strlen(ROUTER_LIST_PREFIX));
                std::string delimiter = ";";
                std::string ipReceive = std::string(inet_ntoa(receiveAddr.sin_addr));

                while (routerList.size() > 0)
                {
                    // separando o ip e a métrica
                    int posIP = routerList.find(delimiter);
                    std::string router = routerList.substr(0, posIP);

                    // separando a métrica para fazer o incremento
                    int posMetric = router.find("-");
                    std::string ip = router.substr(0, posMetric);
                    std::string metricString = router.substr(posMetric + 1);
                    int metric = std::stoi(metricString);

                    // verificando se o ip já está na lista e caso não esteja, adicionando na lista e com a métrica maior
                    bool found = false;
                    if (ipList.size() > 0)
                    {
                        for (int i = 0; i < ipList.size(); i++)
                        {

                            if (ipReceive == vizinhos[i].first)
                            {
                                vizinhos[i].second = 35;
                            }

                            if (ipList[i].second.first == ip || ip == localIp)
                            {
                                found = true;
                                std::cout << "encontrou ip ou localIP" << std::endl;
                                if (metric < ipList[i].second.second)
                                {
                                    ipList[i].second.first = metric;
                                    ipList[i].first = std::string(inet_ntoa(receiveAddr.sin_addr));
                                    std::cout << "Metric updated: " << ip << std::endl;
                                }
                            }
                        }
                    }

                    if (!found)
                    {

                        routerList.erase(0, posIP + delimiter.length());
                        std::cout << "New router added: " << ip << std::endl;
                    }
                }
            }
            else if (message.rfind(MSG_PREFIX, 0) == 0)
            {
                std::string userMessage = message.substr(strlen(MSG_PREFIX));
                std::cout << userMessage << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// void sendMessage(int sockfd, struct sockaddr_in &routerAddr, socklen_t &addrLen, std::string &message)
// {
//     while (true)
//     {
//         getline(std::cin, message);
//         for (int i = 0; i < ipList.size(); i++)
//         {
//             routerAddr.sin_addr.s_addr = inet_addr(ipList[i].first.c_str());
//             int sendLen = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
//             if (sendLen < 0)
//             {
//                 std::cerr << "Error sending message" << std::endl;
//             }
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }

void sendIpList(int sockfd)
{

    struct sockaddr_in routerAddr;
    socklen_t addrLen = sizeof(routerAddr);
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(PORT);

    while (true)
    {
        std::string message = ROUTER_LIST_PREFIX;
        for (int i = 0; i < ipList.size(); i++)
        {
            message += ipList[i].second.first + "-" + std::to_string(ipList[i].second.second) + ";";
        }

        if (ipList.size() > 0)
        {
            for (int i = 0; i < ipList.size(); i++)
            {
                routerAddr.sin_addr.s_addr = inet_addr(ipList[i].first.c_str());
                std::cout << "IpList foi enviada para o ip: " << ipList[i].first << std::endl;
                int sendLen = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
                if (sendLen < 0)
                {
                    std::cerr << "Error sending the ip list" << std::endl;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(15));
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

void countDisconnect()
{
    while (true)
    {
        for (int i = 0; i < vizinhos.size(); i++)
        {
            if (vizinhos[i].second == 0)
            {
                ipList.erase(ipList.begin() + i);
                vizinhos.erase(vizinhos.begin() + i);
                std::cout << "Router " << vizinhos[i].first << " disconnected" << std::endl;
            }

            if (vizinhos[i].second > 0)
            {
                vizinhos[i].second--;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[])
{

    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in routerAddr;
    socklen_t addrLen = sizeof(routerAddr);
    std::string messageUser;
    localIp = argv[1];
    std::cout << "IP Address of user: " << localIp << std::endl;

    // socket();

    sockfd = (socket(AF_INET, SOCK_DGRAM, 0));
    if (sockfd < 0)
    {
        errorFunction("Error to create the socket");
    }

    // routerAddr();
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(PORT);
    routerAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&routerAddr, sizeof(routerAddr)) < 0)
    {
        close(sockfd);
        errorFunction("Error to bind the socket");
    }

    std::ifstream file("roteadores.txt");

    if (!file.is_open())
    {
        std::cerr << "Error opening file: roteadores.txt" << std::endl;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::cout << "Adding router: " << line << std::endl;
        ipList.push_back(std::make_pair(line, std::make_pair(line, 1)));
        vizinhos.push_back(std::make_pair(line, 35));
        std::string msg = '*' + localIp;
        routerAddr.sin_addr.s_addr = inet_addr(line.c_str());
        int sendLen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
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
        std::cout << "IpList size: " << ipList.size() << std::endl;
        // std::cout << "Digite a mensagem: ";
        std::string messageSend = MSG_PREFIX;
        getline(std::cin, messageUser);
        messageSend += ' ' + messageUser;
        std::cout << "Message to the other user: " << messageSend << std::endl;
        for (int i = 0; i < ipList.size(); i++)
        {
            std::cout << "Enviando para: " << ipList[i].first << std::endl;
            routerAddr.sin_addr.s_addr = inet_addr(ipList[i].first.c_str());
            int sendLen = sendto(sockfd, messageUser.c_str(), messageUser.length(), 0, (struct sockaddr *)&routerAddr, addrLen);
            if (sendLen < 0)
            {
                std::cerr << "Error sending message" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
