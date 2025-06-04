#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

typedef struct {
    int clientFileDescriptor;
    struct sockaddr_in clientAddress;
} client_data_t;

int createSocket(int serverPort);

int acceptConnection(int serverFileDescriptor, struct sockaddr_in *clientAddress);

void handleClient(int clientFileDescriptor, struct sockaddr_in *clientAddress);

void* clientHandlerThread(void* arg);

void sendToFurhat(const char *text);

#endif