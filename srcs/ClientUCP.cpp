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

#define PORT 8080
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Hello from client."
#define HEARTBEAT_SIGNAL "HEARTBEAT"
#define HEARTBEAT_TIME 5

void errorFunction(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

void receiveMessages(int sockfd, struct sockaddr_in &serverAddr, socklen_t &addrLen)
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
                continue;
            }
            else
            {
                std::cerr << "Error to receive the message" << std::endl;
            }
        }
        else
        {
            buffer[recvLen] = '\0';
            std::cout << buffer << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(serverAddr);
    std::string userIdentification = " ";
    std::string messageUser;

    // socket()

    sockfd = (socket(AF_INET, SOCK_DGRAM, 0));
    if (sockfd < 0)
    {
        errorFunction("Error to create the socket");
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK); //setting the socket to non-blocking mode

    // set serverAddr

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    // recvfrom(), sendto() and heartbeat -- functions to receive and send messages

    std::thread receiveThread(receiveMessages, sockfd, std::ref(serverAddr), std::ref(addrLen));
    receiveThread.detach();

    while (true)
    {
        if (userIdentification == " ")
        {
            std::cout << "Enter your username: ";
            std::getline(std::cin, messageUser);
            userIdentification = messageUser;

            int sendLen = sendto(sockfd, messageUser.c_str(), messageUser.length(), 0, (struct sockaddr *)&serverAddr, addrLen);
            if (sendLen < 0)
            {
                std::cerr << "Error sending the username" << std::endl;
            }
            
            while(true){
                int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
                if (recvLen < 0){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    } else{
                        std::cerr << "Error to receive the connection message" << std::endl;
                        break;
                    }
                } else {
                    buffer[recvLen] = '\0';
                    std::cout << buffer << std::endl;
                    break;
                }
            }

        }
        else
        {
            //std::cout << userIdentification << ": ";
            std::getline(std::cin, messageUser);
        }

        int sendLen = sendto(sockfd, messageUser.c_str(), messageUser.length(), 0, (struct sockaddr *)&serverAddr, addrLen);
        if (sendLen < 0)
        {
            std::cerr << "Error sending message" << std::endl;
        }
    }

    close(sockfd);
}
