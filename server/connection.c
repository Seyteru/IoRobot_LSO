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
        "Mi piace essere al centro dell'attenzione.",
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
        
        // Attendi risposta
        bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
        if (bytesRead <= 0) break;
        buffer[bytesRead] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        LOG_CLIENT_DEBUG(clientAddress, "Received answer to Q%d: %s", questionIndex + 1, buffer);

        int value = atoi(buffer);
        if (value >= 1 && value <= 7) {
            responses[questionIndex] = value;
            questionIndex++;
            LOG_CLIENT_INFO(clientAddress, "Valid response %d for question %d", value, questionIndex);
        } else {
            LOG_CLIENT_WARNING(clientAddress, "Invalid value received: %s. Please enter a number between 1 and 7.", buffer);
        }
    }   

    // Calcolo dell'estroversione
    double score = 0.0;
    for (int i = 0; i < totalQuestions; i++) {
        int response = responses[i];
        if (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) {
            response = 8 - response;
        }
        score += response;
    }
    double extroversion = score / (totalQuestions * 1.0);

    // Determina stile comportamentale
    const char *style;
    const char *behaviorInstructions;
    
    if (extroversion < 3.0) {
        style = "riservato";
        behaviorInstructions = "\"avoidEyeContact\", \"speakSoftly\", \"usePauses\", \"showUncertainty\"";
    } else if (extroversion < 5.0) {
        style = "neutro";
        behaviorInstructions = "\"neutralTone\", \"moderateGaze\", \"balanced\"";
    } else {
        style = "aperto";
        behaviorInstructions = "\"maintainEyeContact\", \"useGestures\", \"enthusiastic\", \"confident\"";
    }

    // Invia il risultato della personalità
    char resultJson[BUFFER_SIZE];
    snprintf(resultJson, sizeof(resultJson),
            "{ \"type\": \"result\", \"personality\": {"
            "\"extroversion\": %.2f, "
            "\"style\": \"%s\", "
            "\"instructions\": [ %s ]"
            "} }\n",
            extroversion, style, behaviorInstructions);

    send(clientFileDescriptor, resultJson, strlen(resultJson), 0);
    LOG_CLIENT_INFO(clientAddress, "Sent personality result: %s", resultJson);

    // Messaggio di transizione personalizzato
    char transitionMsg[BUFFER_SIZE];
    if (strcmp(style, "riservato") == 0) {
        snprintf(transitionMsg, sizeof(transitionMsg), 
            "{ \"type\": \"transition\", \"message\": \"Eh... va bene, continuiamo con calma...\" }\n");
    } else if (strcmp(style, "neutro") == 0) {
        snprintf(transitionMsg, sizeof(transitionMsg), 
            "{ \"type\": \"transition\", \"message\": \"Bene, ora facciamo qualche domanda più rilassante.\" }\n");
    } else {
        snprintf(transitionMsg, sizeof(transitionMsg), 
            "{ \"type\": \"transition\", \"message\": \"Fantastico! Ora parliamo di cose più divertenti!\" }\n");
    }
    send(clientFileDescriptor, transitionMsg, strlen(transitionMsg), 0);

    // Domande di follow-up con comportamento adattivo
    const char *followUpQuestions[] = {
        "Qual è il tuo colore preferito?",
        "Hai un animale domestico?",
        "Ti piace viaggiare?",
        "Qual è la tua stagione preferita?",
        "Pratichi qualche sport?"
    };

    int numFollowUp = sizeof(followUpQuestions) / sizeof(followUpQuestions[0]);

    for (int i = 0; i < numFollowUp; i++) {
        // Aggiungi pause comportamentali per persona riservata
        if (strcmp(style, "riservato") == 0) {
            char pauseJson[BUFFER_SIZE];
            snprintf(pauseJson, sizeof(pauseJson),
                "{ \"type\": \"behavior\", \"action\": \"pause\", \"duration\": %d }\n", 
                1000 + (rand() % 1500)); // Pause casuali 1-2.5 secondi
            send(clientFileDescriptor, pauseJson, strlen(pauseJson), 0);
        }

        // Invia la domanda con metadati comportamentali
        char askJson[BUFFER_SIZE];
        snprintf(askJson, sizeof(askJson),
            "{ \"type\": \"ask\", \"question\": \"%s\", \"style\": \"%s\", \"questionNum\": %d }\n",
            followUpQuestions[i], style, i + 1);
        send(clientFileDescriptor, askJson, strlen(askJson), 0);
        LOG_CLIENT_INFO(clientAddress, "Sent follow-up question: %s", followUpQuestions[i]);

        bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
        if (bytesRead <= 0) break;
        buffer[bytesRead] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        LOG_CLIENT_INFO(clientAddress, "Received answer: %s", buffer);

        // Invia reazione comportamentale alla risposta
        char reactionJson[BUFFER_SIZE];
        if (strcmp(style, "riservato") == 0) {
            const char* reactions[] = {"Ah, capisco...", "Mmm, interessante.", "Sì, va bene."};
            snprintf(reactionJson, sizeof(reactionJson),
                "{ \"type\": \"reaction\", \"message\": \"%s\", \"style\": \"hesitant\" }\n",
                reactions[rand() % 3]);
        } else if (strcmp(style, "aperto") == 0) {
            const char* reactions[] = {"Che bello!", "Davvero interessante!", "Fantastico!"};
            snprintf(reactionJson, sizeof(reactionJson),
                "{ \"type\": \"reaction\", \"message\": \"%s\", \"style\": \"enthusiastic\" }\n",
                reactions[rand() % 3]);
        } else {
            snprintf(reactionJson, sizeof(reactionJson),
                "{ \"type\": \"reaction\", \"message\": \"Interessante.\", \"style\": \"neutral\" }\n");
        }
        send(clientFileDescriptor, reactionJson, strlen(reactionJson), 0);
    }

    // Messaggio di chiusura
    char closingJson[BUFFER_SIZE];
    if (strcmp(style, "riservato") == 0) {
        snprintf(closingJson, sizeof(closingJson),
            "{ \"type\": \"closing\", \"message\": \"Eh... grazie per aver parlato con me.\" }\n");
    } else if (strcmp(style, "aperto") == 0) {
        snprintf(closingJson, sizeof(closingJson),
            "{ \"type\": \"closing\", \"message\": \"È stato fantastico parlare con te! Grazie mille!\" }\n");
    } else {
        snprintf(closingJson, sizeof(closingJson),
            "{ \"type\": \"closing\", \"message\": \"Grazie per la conversazione.\" }\n");
    }
    send(clientFileDescriptor, closingJson, strlen(closingJson), 0);
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
