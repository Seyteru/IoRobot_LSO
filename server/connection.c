#include "connection.h"
#include "logger.h"
#include <curl/curl.h>

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
    
    // Domande sulla personalità (Likert 1-7)
    const char *personalityQuestions[] = {
        "Mi sento a mio agio in mezzo alla gente.",
        "Preferisco attività solitarie rispetto a quelle di gruppo.",
        "Tendo a prendere l'iniziativa nelle conversazioni.",
        "Evito le situazioni sociali nuove.",
        "Mi sento energico dopo aver socializzato.",
        "Preferisco ascoltare piuttosto che parlare.",
        "Parlo spesso con sconosciuti.",
        "Trovo stressanti le situazioni affollate.",
        "Mi piace essere al centro dell’attenzione.",
        "Ho difficoltà a esprimere le mie emozioni agli altri."
    };

    int responses[10] = {0};
    int totalQuestions = 10;
    int questionIndex = 0;

    // Invia le domande una alla volta
    while (questionIndex < totalQuestions) {
        char askBuffer[BUFFER_SIZE];
        snprintf(askBuffer, sizeof(askBuffer),
                "{\"type\":\"ask\",\"question\":\"%s (1=Disaccordo, 7=Accordo)\"}\n",
                personalityQuestions[questionIndex]);
        send(clientFileDescriptor, askBuffer, strlen(askBuffer), 0);
        LOG_CLIENT_DEBUG(clientAddress, "Sent ask #%d: %s", questionIndex + 1, personalityQuestions[questionIndex]);

        // Invia anche a Furhat
        sendToFurhat(personalityQuestions[questionIndex]);

        // Attendi risposta
        bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
        if (bytesRead <= 0) break;

        buffer[bytesRead] = '\0';
        LOG_CLIENT_DEBUG(clientAddress, "Received answer to Q%d: %s", questionIndex + 1, buffer);

        // Estrai risposta numerica
        char *start = strstr(buffer, "\"answer\":\"");
        if (start) {
            start += strlen("\"answer\":\"");
            char *end = strchr(start, '"');
            if (end) *end = '\0';
            int value = atoi(start);
            if (value >= 1 && value <= 7) {
                responses[questionIndex] = value;
                questionIndex++;
            } else {
                LOG_CLIENT_WARNING(clientAddress, "Invalid value received: %s", start);
            }
        }
    }   

    // Calcolo semplice di estroversione (invertiamo alcune domande)
    double score = 0.0;
    for (int i = 0; i < totalQuestions; i++) {
        int response = responses[i];
        // Inverti domande 2, 4, 6, 8, 10
        if (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) {
            response = 8 - response; // 1 ↔ 7
        }
        score += response;
    }
    double extroversion = score / (totalQuestions * 1.0);

    // Prepara il JSON con il risultato
    char resultJson[BUFFER_SIZE];
    snprintf(resultJson, sizeof(resultJson),
            "{\"type\":\"result\",\"personality\":{"
            "\"extroversion\":%.2f}}",
            extroversion);
    send(clientFileDescriptor, resultJson, strlen(resultJson), 0);
    LOG_CLIENT_INFO(clientAddress, "Sent personality result: %s", resultJson);

    // Comunica il risultato a Furhat
    char furhatJson[512];
    snprintf(furhatJson, sizeof(furhatJson),
         "{\"type\":\"say\", \"text\":\"%s\"}", personalityQuestions[questionIndex]);
    sendToFurhat(furhatJson);
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

void sendToFurhat(const char *jsonMessage) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5001);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection to Python bridge failed");
        close(sockfd);
        return;
    }

    send(sockfd, jsonMessage, strlen(jsonMessage), 0);
    close(sockfd);
}