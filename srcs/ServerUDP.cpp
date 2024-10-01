#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <map>

#define PORT 20800
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Connected to the server || /FILE to send a file"
#define FILE_NOTIFICATION "FILE-TRANSFER"
#define FILE_COMPLETE "FILE-COMPLETE"

//FUNÇÃO PARA EXIBIR ERRO

void errorFunction(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

//FUNÇÃO PARA PEGAR O ENDEREÇO DO CLIENT

std::string getAddressString(const struct sockaddr_in &address)
{
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
    std::map<std::string, struct sockaddr_in> ClientsAddress;
    bool newclient = false;
    std::string newClientMessage;
    bool fileTransfer = false;

    // CRIAÇÃO DO SOCKET

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        errorFunction("Error to create the socket");
    }

    // CONFIGURANDO SERVERADDR

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // CONFIGURANDO O BIND DO SOCKET

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(sockfd);
        errorFunction("Error to bind the socket");
    }

    // recvfrom() and sendto() -- FUNÇÕES PARA O RECEBIMENTO E ENVIO DE MENSAGENS

    while (true)
    {

        // recvfrom() -- ENVIO DE MENSAGENS
        int sendvLen;
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (recvLen < 0)
        {
            std::cerr << "Error receiving data" << std::endl;
            continue;
        }

        buffer[recvLen] = '\0';

        std::string addressClient = getAddressString(clientAddr);

        if (Clients.find(addressClient) == Clients.end())
        {
            newclient = true;
            Clients[addressClient] = buffer;
            ClientsAddress[addressClient] = clientAddr;
            sendvLen = sendto(sockfd, MESSAGE_SERVER, strlen(MESSAGE_SERVER), 0, (struct sockaddr *)&clientAddr, addrLen);
            if (sendvLen < 0)
            {
                std::cerr << "Error to send welcome message" << std::endl;
            }
            std::cout << "New client added: " << Clients[addressClient] << " | IP: " << addressClient << std::endl;
        }
        else
        {
            std::cout << "Received from client " << Clients[addressClient] << ": " << buffer << std::endl;
        }

        //INFORMANDO QUE TEM NOVO CLIENT

        if (newclient){
            newclient = false;
            newClientMessage = Clients[addressClient] + " has joined the server.";
        }
        else
        {
            if(strcmp(buffer, FILE_NOTIFICATION) == 0){
                newClientMessage = FILE_NOTIFICATION;
                fileTransfer = true;

            }
            else if(strcmp(buffer, FILE_COMPLETE) == 0){
                newClientMessage = FILE_COMPLETE;
                fileTransfer = false;
            }
            else{
                if(fileTransfer){
                    newClientMessage = buffer;
                }
                else{
                newClientMessage = Clients[addressClient] + ": " + buffer;
                }
            }
        }



        // sendto() -- ENVIANDO MENSAGENS
        if (Clients.size() > 1)
        {
            for (auto &client : Clients)
            {
                if (client.first != addressClient)
                {
                    struct sockaddr_in targetAddr = ClientsAddress[client.first];

                    sendvLen = sendto(sockfd, newClientMessage.c_str(), newClientMessage.length(), 0, (struct sockaddr *)&targetAddr, addrLen);
                    if (sendvLen < 0)
                    {
                        std::cerr << "Error to send message" << std::endl;
                    }
                }
            }
        }
    }

    // close()

    close(sockfd);
}