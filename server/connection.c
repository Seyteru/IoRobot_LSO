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
    char buffer[BUFFER_SIZE];
    int bytesRead;
    int step = 0;
    char name[100] = {0};
    int age = 0;
    
    // 1) Invia la prima domanda al client (robot)
    const char *firstAsk = "{\"type\":\"ask\",\"question\":\"Come ti chiami?\"}\n";
    send(clientFileDescriptor, firstAsk, strlen(firstAsk), 0);
    LOG_CLIENT_DEBUG(clientAddress, "Sent ask #1: Come ti chiami?");

    // 2) Ciclo di lettura per gestire le risposte del client
    while ((bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytesRead] = '\0';
        LOG_CLIENT_DEBUG(clientAddress, "Received: %s", buffer);

        // Verifica se è un messaggio di tipo "answer"
        if (strstr(buffer, "\"type\":\"answer\"")) {
            if (step == 0) {
                // Estrai il nome dal JSON
                // Formato previsto: {"type":"answer","answer":"Mario"}
                char *start = strstr(buffer, "\"answer\":\"");
                if (start) {
                    start += strlen("\"answer\":\"");      // posizionati subito dopo la chiave
                    char *end = strchr(start, '"');        // trova la virgoletta di chiusura
                    if (end) *end = '\0';                  // tronca la stringa
                    strncpy(name, start, sizeof(name)-1);  // copia nel tuo buffer
                    name[sizeof(name)-1] = '\0';
                    LOG_CLIENT_INFO(clientAddress, "User name: %s", name);
                }

                // Invia la seconda domanda
                const char *secondAsk = "{\"type\":\"ask\",\"question\":\"Quanti anni hai?\"}\n";
                send(clientFileDescriptor, secondAsk, strlen(secondAsk), 0);
                LOG_CLIENT_DEBUG(clientAddress, "Sent ask #2: Quanti anni hai?");
                step = 1;
            }
            else if (step == 1) {
                // Estrai l'età dal JSON
                // Formato previsto: {"type":"answer","answer":"30"}
                char *start = strstr(buffer, "\"answer\":\"");
                if (start) {
                    // Mi sposto subito dopo la parte "\"answer\":\""
                    start += strlen("\"answer\":\"");
                    // Trovo la virgoletta di chiusura
                    char *end = strchr(start, '"');
                    if (end) {
                        *end = '\0';              // Termino la stringa lì
                    }
                    // Converto la sottostringa in intero
                    int age = atoi(start);
                    LOG_CLIENT_INFO(clientAddress, "User age: %d", age);
                }

                // Elabora i dati per la personalità (dummy)
                double extroversion = 0.7;
                double friendliness = 0.8;

                // Prepara il messaggio di risultato
                char resultJson[BUFFER_SIZE];
                snprintf(resultJson, sizeof(resultJson),
                         "{\"type\":\"result\",\"personality\":{"
                         "\"extroversion\":%.2f,\"friendliness\":%.2f}}",
                         extroversion, friendliness);

                // Invia il risultato al client
                send(clientFileDescriptor, resultJson, strlen(resultJson), 0);
                LOG_CLIENT_DEBUG(clientAddress, "Sent personality result: %s", resultJson);

                // Terminare la sessione
                break;
            }
        }
        else {
            LOG_CLIENT_WARNING(clientAddress, "Unexpected message format: %s", buffer);
        }
    }

    if (bytesRead == 0) {
        LOG_CLIENT_INFO(clientAddress, "Client disconnected normally");
    } else if (bytesRead < 0) {
        LOG_CLIENT_ERROR(clientAddress, "Read error: %s", strerror(errno));
    }

    // Chiudi connessione
    close(clientFileDescriptor);
    LOG_CLIENT_DEBUG(clientAddress, "Connection closed");
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