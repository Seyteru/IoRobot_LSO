#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int createSocket(int serverPort);

int acceptConnection(int serverFileDescriptor);

void handleClient(int clientFileDescriptor);

#endif