#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <map>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Connected to the server."

void errorFunction(const std::string &message){
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

std::string getAddressString(const struct sockaddr_in &address){
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip, INET_ADDRSTRLEN);
    return std::string(ip) + ":" + std::to_string(ntohs(address.sin_port));
}

int main()
{
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    std::map<std::string, std::string> Clients;

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
        int sendvLen;
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (recvLen < 0)
        {
            std::cerr << "Error receiving data" << std::endl;
            continue;
        }

        buffer[recvLen] = '\0'; // Null-terminate the received data
        std::cout << "Received from client: " << buffer << std::endl;

        std::string addressClient = getAddressString(clientAddr);

        if(Clients.find(addressClient) == Clients.end()){
            Clients[addressClient] = buffer;
            sendvLen = sendto(sockfd, MESSAGE_SERVER, strlen(MESSAGE_SERVER), 0, (struct sockaddr *)&clientAddr, addrLen);
            if(sendvLen < 0){
                std::cerr << "Error to send welcome message" << std::endl;
            }
            std::cout << "New client added: " << buffer << " | IP: " << addressClient << std::endl;
        }



        // sendto() -- send messages
        
        int sendLen = sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, addrLen);
        
    }

    // close()

    close(sockfd);
}