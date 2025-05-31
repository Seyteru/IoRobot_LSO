#include "server.h"
#include "connection.h"
#include "logger.h"
#include <errno.h>

int serverFileDescriptor;
volatile int serverRunning = 1;

int initializeServer(){
    LOG_INFO("Server Initialization on Port: 5555");
    serverFileDescriptor = createSocket(5555);

    if(serverFileDescriptor < 0){
        return -1;
    }

    return 0;
}

void runServer(){
    pthread_attr_t threadAttr;
    int attr_result;
    
    if ((attr_result = pthread_attr_init(&threadAttr)) != 0) {
        LOG_ERROR("Failed to initialize thread attributes: %s", strerror(attr_result));
        return;
    }
    
    if ((attr_result = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED)) != 0) {
        LOG_ERROR("Failed to set detached thread state: %s", strerror(attr_result));
        pthread_attr_destroy(&threadAttr);
        return;
    }
    
    while(serverRunning){
        LOG_INFO("Waiting for Connection...");
        
        struct sockaddr_in clientAddress;
        int clientFileDescriptor = acceptConnection(serverFileDescriptor, &clientAddress);

        if(clientFileDescriptor < 0){
            LOG_ERROR("Connection Failure!");
            continue;
        }

        LOG_CLIENT_INFO(&clientAddress, "Connection Successful");
        
        client_data_t* clientData = malloc(sizeof(client_data_t));
        if (!clientData) {
            LOG_ERROR("Failed to allocate memory for client data: %s", strerror(errno));
            close(clientFileDescriptor);
            continue;
        }
        
        clientData->clientFileDescriptor = clientFileDescriptor;
        clientData->clientAddress = clientAddress;
        
        pthread_t threadId;
        int result = pthread_create(&threadId, &threadAttr, clientHandlerThread, (void*)clientData);
        
        if (result != 0) {
            LOG_CLIENT_ERROR(&clientAddress, "Failed to create thread: %s", strerror(result));
            close(clientFileDescriptor);
            free(clientData);
        } else {
            LOG_CLIENT_DEBUG(&clientAddress, "Created new thread with ID %lu for client %s:%d", 
                           (unsigned long)threadId,
                           inet_ntoa(clientAddress.sin_addr), 
                           ntohs(clientAddress.sin_port));
        }
    }
    
    pthread_attr_destroy(&threadAttr);
}

void shutdownServer(){
    serverRunning = 0;
    close(serverFileDescriptor);
    
    LOG_INFO("Server socket closed, waiting for threads to complete");
    sleep(1);
}