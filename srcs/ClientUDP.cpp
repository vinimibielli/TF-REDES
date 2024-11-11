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

#define PORT 9000
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Hello from client."
#define ROUTER_LIST_PREFIX "ROUTER_LIST:"
#define MSG_PREFIX "MSG:"

void errorFunction(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

void receiveMessage(int sockfd, struct sockaddr_in &serverAddr, socklen_t &addrLen, std::vector<std::pair<std::string, std::pair<int, std::string>>> &ipList)
{
    char buffer[BUFFER_SIZE];
    while (true)
    {
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
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
                    while(routerList.size() > 0)
                    {
                        //separando o ip e a métrica
                        int posIP = routerList.find(delimiter);
                        std::string router = routerList.substr(0, posIP);

                        //separando a métrica para fazer o incremento
                        int posMetric = router.find("-");
                        std::string ip = router.substr(0, posMetric);
                        std::string metricString = router.substr(posMetric + 1);
                        int metric = std::stoi(metricString);


                        //verificando se o ip já está na lista e caso não esteja, adicionando na lista e com a métrica maior
                        bool found = false;
                        if(ipList.size() > 0)
                        {
                            for(int i = 0; i < ipList.size(); i++)
                            {
                                if(ipList[i].second.second == ip)
                                {
                                    found = true;
                                    if(metric < ipList[i].second.first)
                                    {
                                        ipList[i].second.first = metric;
                                        ipList[i].first = std::string(inet_ntoa(serverAddr.sin_addr));
                                        std::cout << "Metric updated: " << ip << std::endl;
                                    }
                                    
                                }
                            }
                        }

                        if(!found)
                        {
                        ipList.push_back(make_pair(std::string(inet_ntoa(serverAddr.sin_addr)), make_pair(metric + 1, ip)));
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

void sendMessage(int sockfd, struct sockaddr_in &serverAddr, socklen_t &addrLen, std::string &message)
{
    while(true)
    {
        getline(std::cin, message);
        int sendLen = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr *)&serverAddr, addrLen);
        if (sendLen < 0)
        {
            std::cerr << "Error sending message" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void sendIpList(int sockfd, struct sockaddr_in &serverAddr, socklen_t &addrLen, std::vector<std::pair<std::string, std::pair<int, std::string>>> &ipList)
{
    std::string message = ROUTER_LIST_PREFIX;
    for (int i = 0; i < ipList.size(); i++)
    {
        message += ipList[i].second.second + "-" + std::to_string(ipList[i].second.first) + ";";

    }

    int sendLen = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr *)&serverAddr, addrLen);
    if (sendLen < 0)
    {
        std::cerr << "Error sending the ip list" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(15));
}

void printIpList(std::vector<std::pair<std::string, std::pair<int, std::string>>> &ipList)
{
    for (int i = 0; i < ipList.size(); i++)
    {
        std::cout << ipList[i].first << " - " << ipList[i].second.first << " - " << ipList[i].second.second << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(25));
}

void clientUDP(){

    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(serverAddr);
    std::string userIdentification = " ";
    std::string messageUser;
    int sendLen;
    std::vector<std::pair<std::string, std::pair<std::string, int>>> ipList;



    // Read from file

    std::vector<std::string> result;
    std::ifstream file("roteadores.txt");

    if (!file.is_open()) {
        std::cerr << "Error opening file: roteadores.txt" << std::endl;
    }

    std::string line;
    while (std::getline(file, line)) {
        ipList.push_back(make_pair(line, make_pair(line, 1)));
    }

    file.close();

    std::thread sendIpListThread(sendIpList, sockfd, std::ref(serverAddr), std::ref(addrLen), std::ref(ipList));
    sendIpListThread.detach();
    std::thread printIpListThread(printIpList, std::ref(ipList));
    printIpListThread.detach();

    // socket()

    sockfd = (socket(AF_INET, SOCK_DGRAM, 0));
    if (sockfd < 0)
    {
        errorFunction("Error to create the socket");
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK); // setting the socket to non-blocking mode

    // set serverAddr

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    // recvfrom() and sendto()
    std::thread receiveThread(receiveMessage, sockfd, std::ref(serverAddr), std::ref(addrLen), std::ref(ipList));
    receiveThread.detach();

    std::thread sendThread(sendMessage, sockfd, std::ref(serverAddr), std::ref(addrLen), std::ref(messageUser));
    sendThread.detach();
    

    close(sockfd);
}

void serverUDP(){
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    std::vector<std::pair<std::string, std::pair<int, std::string>>> ipList;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        errorFunction("Error creating socket");
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        errorFunction("Error binding socket");
    }

    // Start receiving messages
    std::thread receiveThread(receiveMessage, sockfd, std::ref(clientAddr), std::ref(addrLen), std::ref(ipList));
    receiveThread.detach();

    // Periodically send IP list
    while (true)
    {
        sendIpList(sockfd, clientAddr, addrLen, ipList);
    }

    close(sockfd);
}

int main(){

    std::thread serverThread(serverUDP);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread clientThread(clientUDP);

    serverThread.join();
    clientThread.join();

    return 0;
}