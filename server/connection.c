#include "connection.h"
#include "logger.h"

#define BUFFER_SIZE 1024

int createSocket(int serverPort){
    int serverFileDescriptor;
    struct sockaddr_in address;
    serverFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    
    if(serverFileDescriptor < 0){
        LOG_ERROR("Socket Creation Failure: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    if (setsockopt(serverFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("setsockopt failed: %s", strerror(errno));
        close(serverFileDescriptor);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(serverPort);

    if(bind(serverFileDescriptor, (struct sockaddr *)&address, sizeof(address)) < 0){
        LOG_ERROR("Binding Failure: %s", strerror(errno));
        close(serverFileDescriptor);
        return -1;
    }

    if(listen(serverFileDescriptor, 10) < 0){
        LOG_ERROR("Listening Failure: %s", strerror(errno));
        close(serverFileDescriptor);
        return -1;
    }

    struct sockaddr_in actualAddress;
    socklen_t addressLength = sizeof(actualAddress);
    if(getsockname(serverFileDescriptor, (struct sockaddr *)&actualAddress, &addressLength) == 0){
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &actualAddress.sin_addr, ipStr, sizeof(ipStr));
        LOG_INFO("Server listening on IP: %s, Port: %d", ipStr, serverPort);
    } else {
        LOG_WARNING("Couldn't retrieve server socket information: %s", strerror(errno));
        LOG_INFO("Server started but couldn't determine the listening IP");
    }

    return serverFileDescriptor;
}

int acceptConnection(int serverFileDescriptor, struct sockaddr_in *clientAddress){
    socklen_t addressLength = sizeof(*clientAddress);

    int clientFileDescriptor = accept(serverFileDescriptor, (struct sockaddr *)clientAddress, &addressLength);
    if(clientFileDescriptor < 0){
        LOG_ERROR("Accepting Connection Failure: %s", strerror(errno));
        return -1;
    }

    LOG_CLIENT_INFO(clientAddress, "Client connected");
    return clientFileDescriptor;
}

void handleClient(int clientFileDescriptor, struct sockaddr_in *clientAddress){
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
    
    if(bytesRead > 0){
        LOG_CLIENT_DEBUG(clientAddress, "Received: %s", buffer);
        const char *response = "WELCOME FROM SERVER"; // Handle the response here
        send(clientFileDescriptor, response, strlen(response), 0);
        LOG_CLIENT_DEBUG(clientAddress, "Response sent");
    } else if(bytesRead == 0){
        LOG_CLIENT_INFO(clientAddress, "Client disconnected");
    } else{
        LOG_CLIENT_ERROR(clientAddress, "Read Error: %s", strerror(errno));
    }
}

void* clientHandlerThread(void* arg) {
    client_data_t* data = (client_data_t*)arg;
    pthread_t tid = pthread_self();
    
    LOG_CLIENT_DEBUG(&data->clientAddress, "Thread %lu started for client %s:%d", 
                     (unsigned long)tid,
                     inet_ntoa(data->clientAddress.sin_addr), 
                     ntohs(data->clientAddress.sin_port));
    
    handleClient(data->clientFileDescriptor, &data->clientAddress);
    
    close(data->clientFileDescriptor);
    
    LOG_CLIENT_DEBUG(&data->clientAddress, "Thread %lu closing for client %s:%d", 
                    (unsigned long)tid,
                    inet_ntoa(data->clientAddress.sin_addr), 
                    ntohs(data->clientAddress.sin_port));
    
    free(data);
    
    pthread_exit(NULL);
}