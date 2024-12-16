#include "connection.h"

#define BUFFER_SIZE 1024

int createSocket(int serverPort){
    int serverFileDescriptor;
    struct sockaddr_in address;
    serverFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    
    if(serverFileDescriptor == 0){
        perror("Socket Creation Failure!");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(serverPort);

    if(bind(serverFileDescriptor, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("Binding Failure!");
        close(serverFileDescriptor);
        return -1;
    }

    if(listen(serverFileDescriptor, 3) < 0){
        perror("Listening Failure!");
        close(serverFileDescriptor);
        return -1;
    }

    struct sockaddr_in actualAddress;
    socklen_t addressLength = sizeof(actualAddress);
    if(getsockname(serverFileDescriptor, (struct sockaddr *)&actualAddress, &addressLength) == 0){
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &actualAddress.sin_addr, ipStr, sizeof(ipStr));
        printf("Server listening on IP: %s, Port: %d...\n", ipStr, serverPort);
    }

    return serverFileDescriptor;
}

int acceptConnection(int serverFileDescriptor){
    struct sockaddr_in address;
    socklen_t addressLength = sizeof(address);

    int clientFileDescriptor = accept(serverFileDescriptor, (struct sockaddr *)&address, &addressLength);
    if(clientFileDescriptor < 0){
        perror("Accepting Connection Failure!");
        return -1;
    }

    printf("Client connected from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    return clientFileDescriptor;
}

void handleClient(int clientFileDescriptor){
    char buffer[BUFFER_SIZE] = {0};
    int bytedRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
    
    if(bytedRead > 0){
        printf("Received: %s\n", buffer);
        const char *response = "WELCOME FROM SERVER";
        send(clientFileDescriptor, response, strlen(response), 0);
        printf("Response sent\n");
    } else if(bytedRead == 0){
        printf("Client disconnected");
    } else{
        perror("Read Error!");
    }

    close(clientFileDescriptor);
}
