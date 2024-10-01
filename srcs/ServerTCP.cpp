#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <thread>
#include <mutex>

#define PORT 20800
#define BUFFER_SIZE 1024
#define MESSAGE_SERVER "Connected to the server || /FILE to send a file"
#define FILE_NOTIFICATION "FILE-TRANSFER"
#define FILE_COMPLETE "FILE-COMPLETE"
#define USERNAME_INFORMATION "Please inform your username: "

//FUNÇÃO PARA INFORMAR O ERRO

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

//FUNÇÃO PARA LIDAR COM O CLIENT A PARTIR DE UMA THREAD

void handleClient(int clientSockfd, struct sockaddr_in clientAddr, std::map<int, std::string> &clients, std::mutex &clientsMutex)
{
    char buffer[BUFFER_SIZE];
    bool fileTransfer = false;
    std::string newClientMessage;
    int sendLen;
    int recvLen;

    std::string addressClient = getAddressString(clientAddr);

    sendLen = send(clientSockfd, USERNAME_INFORMATION, strlen(USERNAME_INFORMATION), 0); //PEDE PARA O CLIENT O USUÁRIO
    if (sendLen < 0)
    {
        std::cerr << "Error sending username message" << std::endl;
    }

    recvLen = recv(clientSockfd, buffer, BUFFER_SIZE, 0);
    buffer[recvLen] = '\0';

    {
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients[clientSockfd] = buffer;
    }

    // ENVIA MENSAGEM INFORMANDO PARA O SERVER QUE ESTÁ CONECTADO

    sendLen = send(clientSockfd, MESSAGE_SERVER, strlen(MESSAGE_SERVER), 0);
    if (sendLen < 0)
    {
        std::cerr << "Error sending welcome message" << std::endl;
    }
    std::cout << "New client " << clients[clientSockfd] << " added: " << addressClient << std::endl;

    // INFORMA OS OUTROS USUÁRIOS QUE UM NOVO ENTROU NO SERVER
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        newClientMessage = clients[clientSockfd] + " has joined the server.";
        for (const auto &client : clients)
        {
            if (client.first != clientSockfd)
            {
                sendLen = send(client.first, newClientMessage.c_str(), newClientMessage.length(), 0);
                if (sendLen < 0)
                {
                    std::cerr << "Error sending message to client: " << client.second << std::endl;
                }
            }
        }
    }

    //RECEBE E ENVIA MENSAGENS PARA OS CLIENTS

    while (true)
    {
        recvLen = recv(clientSockfd, buffer, BUFFER_SIZE, 0);
        if (recvLen <= 0)
        {
            if (recvLen == 0)
            {
                std::cout << "Client disconnected: " << addressClient << std::endl; //INFORMA AO SERVIDOR QUE O CLIENT DESCONECTOU
            }
            else
            {
                std::cerr << "Error receiving data from client: " << addressClient << std::endl;
            }
            close(clientSockfd);
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                newClientMessage = clients[clientSockfd] + " has left the server."; //INFORMA OS OUTROS USUÁRIOS QUE UM ESPECÍFICO DESCONECTOU
                for (const auto &client : clients)
                {
                    if (client.first != clientSockfd)
                    {
                        sendLen = send(client.first, newClientMessage.c_str(), newClientMessage.length(), 0);
                        if (sendLen < 0)
                        {
                            std::cerr << "Error sending message to client: " << client.second << std::endl;
                        }
                    }
                }
                clients.erase(clientSockfd);
            }
            break;
        }

        buffer[recvLen] = '\0';

        std::cout << "Received from client " << clients[clientSockfd] << ": " << buffer << std::endl;

        if (strcmp(buffer, FILE_NOTIFICATION) == 0) //INFORMA QUE UM ARQUIVO ESTÁ SENDO ENVIADO
        {
            newClientMessage = FILE_NOTIFICATION;
            fileTransfer = true;
        }
        else if (strcmp(buffer, FILE_COMPLETE) == 0) //INFORMA QUE O ARQUIVO ACABOU DE SER ENVIADO
        {
            newClientMessage = FILE_COMPLETE;
            fileTransfer = false;
        }
        else
        {
            if (fileTransfer) //ARQUIVO ESTÁ SENDO ENVIADO
            {
                newClientMessage = buffer;
            }
            else
            {
                newClientMessage = clients[clientSockfd] + ": " + buffer;
            }
        }

        // ENVIA AS MENSAGENS PARA OS OUTROS CLIENTS

        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto &client : clients)
        {
            if (client.first != clientSockfd)
            {
                sendLen = send(client.first, newClientMessage.c_str(), newClientMessage.length(), 0);
                if (sendLen < 0)
                {
                    std::cerr << "Error sending message to client: " << client.second << std::endl;
                }
            }
        }
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    char buffer[BUFFER_SIZE];
    std::map<int, std::string> Clients;
    std::mutex clientsMutex;

    // socket() -- CRIA O SOCKET

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        errorFunction("Error to create the socket");
    }

    // CONFIGURA O SERVERADDR

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // CONFIGURA O SOCKET COM O ENDEREÇO DELE A PARTIR DO BIND()

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(sockfd);
        errorFunction("Error to bind the socket");
    }

    //FUNÇÃO LISTEN()

    if(listen(sockfd, 5) < 0){
        close(sockfd);
        errorFunction("Error to listen the socket");
    }

    std::cout << "Server listening on port " << PORT << std::endl;


    while (true)
     {
        int clientSockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSockfd < 0)
        {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        std::thread clientThread(handleClient, clientSockfd, clientAddr, std::ref(Clients), std::ref(clientsMutex)); //CRIA UMA THREAD PARA CADA CLIENT
        clientThread.detach();
    }


    close(sockfd);
    return 0;
}