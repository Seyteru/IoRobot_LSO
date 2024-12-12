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

    return serverFileDescriptor;
}

int acceptConnection(int serverFileDescriptor){
    struct sockaddr_in address;
    int addressLength = sizeof(address);
    return accept(serverFileDescriptor, (struct sockaddr *)&address, (socklen_t *)&addressLength);
}

void handleClient(int clientFileDescriptor){
    char buffer[BUFFER_SIZE] = {0};
    read(clientFileDescriptor, buffer, BUFFER_SIZE);
    printf("Received: %s\n", buffer);
    const char *response = "WELCOME FROM SERVER!";
    send(clientFileDescriptor, response, strlen(response), 0);
    printf("Response sent\n");
}
