#ifndef CONNECTION_H
#define CONNECTION_H

int createSocket(int serverPort);

int acceptConnection(int serverFileDescriptor);

void handleClient(int clientFileDescriptor);

#endif