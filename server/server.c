#include "server.h"
#include "connection.h"

int serverFileDescriptor;

int initializeServer(){
    serverFileDescriptor = createSocket(8080);

    if(serverFileDescriptor < 0){
        return -1;
    }

    printf("Server Initialization on Port: 8080\n");
    return 0;
}

void runServer(){
    while(1){
        printf("Waiting for Connection...\n");
        int clientFileDescriptor = acceptConnection(serverFileDescriptor);

        if(clientFileDescriptor < 0){
            fprintf(stderr, "Connection Failure!");
            continue;
        }

        printf("Connection Successful!");
        handleClient(clientFileDescriptor);
        close(clientFileDescriptor);
    }
}

void shutdownServer(){
    close(serverFileDescriptor);
}
