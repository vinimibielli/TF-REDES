#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Hello from server."

void errorFunction(const std::string &message){
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

int main()
{
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // socket() -- create socket

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        errorFunction("Error to create the socket");
    }

    // set serverAddr -- setting up the servers address structures

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // bind() -- binding the socket to the server address

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(sockfd);
        errorFunction("Error to bind the socket");
    }

    // recvfrom() and sendto() -- functions to receive and send messages

    while (true)
    {
        // recvfrom() -- receive messages
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (recvLen < 0)
        {
            std::cerr << "Error receiving data" << std::endl;
            continue;
        }

        buffer[recvLen] = '\0'; // Null-terminate the received data
        std::cout << "Received from client: " << buffer << std::endl;

        // sendto() -- send messages
        std::string messageUser;
        std::cout << "Enter Message to server: ";
        std::getline(std::cin, messageUser);
        int sendLen = sendto(sockfd, messageUser.c_str(), messageUser.length(), 0, (struct sockaddr *)&clientAddr, addrLen);
        if (sendLen < 0)
        {
            std::cerr << "Error sending data" << std::endl;
            continue;
        }
    }

    // close()

    close(sockfd);
}