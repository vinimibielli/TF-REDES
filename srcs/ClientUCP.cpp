#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Hello from client."

void errorFunction(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
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

    // set serverAddr

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    // recvfrom() and sendto()

    while (true)
    {

        if (userIdentification == " ")
        {
            std::cout << "Enter your username: ";
            std::getline(std::cin, messageUser);
            userIdentification = messageUser;   
        }
        else
        {
            std::cout << "Enter Message to server: ";
            std::getline(std::cin, messageUser);
        }

        int sendLen = sendto(sockfd, messageUser.c_str(), messageUser.length(), 0, (struct sockaddr *)&serverAddr, addrLen);
        if (sendLen < 0)
        {
            std::cerr << "Error sending message" << std::endl;
        }

        std::cout << "Message sent to server" << std::endl;

        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
        if (recvLen < 0)
        {
            std::cerr << "Error to receive the message" << std::endl;
        }

        buffer[recvLen] = '\0';
        std::cout << "Received response from server: " << buffer << std::endl;

        // close()
    }

    close(sockfd);
}
