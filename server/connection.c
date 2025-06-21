#include "connection.h"
#include "logger.h"
#include "gpt_client.h"
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
    
    // Domande sulla personalità (Likert 1-7) - mantenute localmente
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
    
    // *** NUOVO: Inizializza sessione GPT ***
    gpt_session_t* gpt_session = gpt_create_session();
    if (!gpt_session) {
        LOG_CLIENT_ERROR(clientAddress, "Failed to create GPT session, falling back to static questions");
    } else {
        // Inizializza la sessione GPT con il prompt di sistema
        char* system_prompt = gpt_load_prompt_template();
        if (!system_prompt || gpt_initialize_session(gpt_session, system_prompt) != 0) {
            LOG_CLIENT_ERROR(clientAddress, "Failed to initialize GPT session, falling back to static questions");
            if (gpt_session) {
                gpt_destroy_session(gpt_session);
                gpt_session = NULL;
            }
        }
        if (system_prompt) free(system_prompt);
    }
    
    double current_extroversion = 3.5; // Valore neutrale iniziale per adattare il comportamento

    // ========== FASE 1: PERSONALITY ASSESSMENT ==========
    // Invia le domande una alla volta (ora generate da GPT)
    while (questionIndex < totalQuestions) {
        char gpt_prompt[BUFFER_SIZE];
        char gpt_response[GPT_RESPONSE_SIZE];
        
        // Determina il comportamento per questa domanda
        const char* behavior_style = determine_behavior_style(current_extroversion, questionIndex, 
                                                             questionIndex > 0 ? buffer : NULL);
        
        // Se GPT è disponibile, genera la domanda dinamicamente
        if (gpt_session && gpt_format_personality_prompt(gpt_prompt, sizeof(gpt_prompt), 
                                         questionIndex == 0 ? NULL : buffer, 
                                         behavior_style, 
                                         personalityQuestions[questionIndex]) == 0 &&
            gpt_send_message(gpt_session, gpt_prompt, gpt_response, sizeof(gpt_response)) == 0) {
              // Sanifica la risposta GPT per il JSON
            char sanitized_response[BUFFER_SIZE];
            sanitize_for_json(gpt_response, sanitized_response, sizeof(sanitized_response));
            
            // Invia la domanda GPT al client
            char askJson[BUFFER_SIZE];
            int written = snprintf(askJson, sizeof(askJson),
                    "{ \"type\": \"gpt_ask\", \"question\": \"%s\", \"questionNum\": %d, \"total\": %d }\n",
                    sanitized_response, questionIndex + 1, totalQuestions);
            if (written < 0 || written >= (int)sizeof(askJson)) {
                fprintf(stderr, "Warning: JSON stringa troncata (%d caratteri)\n", written);
            }
            send(clientFileDescriptor, askJson, strlen(askJson), 0);
            LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent GPT-generated question %d/%d (behavior: %s)", 
                           questionIndex + 1, totalQuestions, behavior_style);
            LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", askJson);
        } else {
            // Fallback alle domande statiche se GPT non funziona
            char askBuffer[BUFFER_SIZE];
            snprintf(askBuffer, sizeof(askBuffer),
                    "{\"type\":\"ask\",\"question\":\"%s\",\"questionNum\":%d,\"total\":%d}\n",
                    personalityQuestions[questionIndex], questionIndex + 1, totalQuestions);
            send(clientFileDescriptor, askBuffer, strlen(askBuffer), 0);
            LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent STATIC question %d/%d (GPT unavailable)", 
                           questionIndex + 1, totalQuestions);
            LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", askBuffer);
        }
        
        // Attendi risposta
        bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
        if (bytesRead <= 0) {
            LOG_CLIENT_ERROR(clientAddress, "← CLIENT: Connection lost or no response received");
            break;
        }
        buffer[bytesRead] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        LOG_CLIENT_INFO(clientAddress, "← CLIENT: Received response to Q%d: \"%s\"", questionIndex + 1, buffer);
        
        int value = atoi(buffer);
        if (value >= 1 && value <= 7) {
            responses[questionIndex] = value;
            questionIndex++;
            
            // Aggiorna l'estroversione parziale per adattare il comportamento GPT
            double partial_score = 0.0;
            for (int i = 0; i < questionIndex; i++) {
                int response = responses[i];
                // Inverti il punteggio per le domande con punteggio inverso (2,4,6,8,10)
                if (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) {
                    response = 8 - response;
                }
                partial_score += response;
            }
            current_extroversion = partial_score / questionIndex;
            
            LOG_CLIENT_INFO(clientAddress, "✓ VALID: Question %d answered with %d (partial extroversion: %.2f)", 
                          questionIndex, value, current_extroversion);
        } else {
            LOG_CLIENT_WARNING(clientAddress, "✗ INVALID: Received \"%s\" - expected number 1-7, asking again", buffer);
        }
    }

    // Calcolo dell'estroversione finale
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
    LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent personality assessment results");
    LOG_CLIENT_INFO(clientAddress, "📊 RESULTS: extroversion=%.2f, style=%s", 
                   extroversion, style);
    LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", resultJson);

    // Messaggio di transizione al follow-up
    char transitionJson[BUFFER_SIZE];
    snprintf(transitionJson, sizeof(transitionJson),
            "{ \"type\": \"state_change\", \"new_state\": \"follow_up\", \"personality\": \"%s\" }\n",
            style);
    send(clientFileDescriptor, transitionJson, strlen(transitionJson), 0);
    LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent state change to follow-up");
    LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", transitionJson);

    // Attendi conferma dal client
    bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
    if (bytesRead <= 0) {
        LOG_CLIENT_ERROR(clientAddress, "← CLIENT: No confirmation received for state change");
        goto cleanup;
    }
    buffer[bytesRead] = '\0';
    buffer[strcspn(buffer, "\r\n")] = '\0';
    
    if (strcmp(buffer, "READY_FOR_FOLLOWUP") != 0) {
        LOG_CLIENT_WARNING(clientAddress, "← CLIENT: Unexpected response: \"%s\", expected READY_FOR_FOLLOWUP", buffer);
        goto cleanup;
    }
    
    LOG_CLIENT_INFO(clientAddress, "← CLIENT: Received confirmation for follow-up phase");

    // ========== FASE 2: FOLLOW-UP CONVERSATION ==========
    if (gpt_session) {
        LOG_CLIENT_INFO(clientAddress, "🔄 Starting follow-up conversation phase");
        
        // Numero di domande di follow-up (configurabile)
        int followup_questions = 3;
        int followup_index = 0;
        
        while (followup_index < followup_questions) {
            char followup_prompt[BUFFER_SIZE];
            char followup_response[GPT_RESPONSE_SIZE];
            
            // Crea il prompt per la domanda di follow-up basata sulla personalità
            int written = snprintf(followup_prompt, sizeof(followup_prompt),
                "[personalità:%s]; [domanda_followup:%d]; [ultima_risposta:%s]; [genera_domanda_approfondimento]",
                style, followup_index + 1, followup_index > 0 ? buffer : "prima_domanda");
            
            if (written < 0 || written >= (int)sizeof(followup_prompt)) {
                fprintf(stderr, "Warning: followup_prompt troncato (%d caratteri)\n", written);
            }
            
            // Genera la domanda di follow-up tramite GPT
            if (gpt_send_message(gpt_session, followup_prompt, followup_response, sizeof(followup_response)) == 0) {
                // Sanifica la risposta GPT per il JSON
                char sanitized_followup[BUFFER_SIZE];
                sanitize_for_json(followup_response, sanitized_followup, sizeof(sanitized_followup));
                
                // Invia la domanda di follow-up al client
                char followupJson[BUFFER_SIZE];
                int written = snprintf(followupJson, sizeof(followupJson),
                        "{ \"type\": \"gpt_ask\", \"question\": \"%s\", \"questionNum\": %d, \"total\": %d }\n",
                        sanitized_followup, followup_index + 1, followup_questions);
                
                if (written < 0 || written >= (int)sizeof(followupJson)) {
                    fprintf(stderr, "Warning: followupJson troncato (%d caratteri)\n", written);
                }
                
                send(clientFileDescriptor, followupJson, strlen(followupJson), 0);
                LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent GPT follow-up question %d/%d", 
                               followup_index + 1, followup_questions);
                LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", followupJson);
                
                // Attendi risposta del client
                bytesRead = read(clientFileDescriptor, buffer, BUFFER_SIZE - 1);
                if (bytesRead <= 0) {
                    LOG_CLIENT_ERROR(clientAddress, "← CLIENT: Connection lost during follow-up");
                    break;
                }
                buffer[bytesRead] = '\0';
                buffer[strcspn(buffer, "\r\n")] = '\0';
                
                LOG_CLIENT_INFO(clientAddress, "← CLIENT: Received follow-up response %d: \"%s\"", 
                               followup_index + 1, buffer);
                
                followup_index++;
            } else {
                LOG_CLIENT_ERROR(clientAddress, "Failed to generate follow-up question %d, ending conversation", 
                               followup_index + 1);
                break;
            }
        }
        
        // Messaggio di chiusura personalizzato tramite GPT
        char final_prompt[512];
        int written = snprintf(final_prompt, sizeof(final_prompt), 
            "[%s]; [%s]; [conversazione_conclusa_completa]", buffer, style);
        
        if (written < 0 || written >= (int)sizeof(final_prompt)) {
            fprintf(stderr, "Warning: final_prompt troncato (%d caratteri)\n", written);
        }
        
        char final_response[GPT_RESPONSE_SIZE];
        if (gpt_send_message(gpt_session, final_prompt, final_response, sizeof(final_response)) == 0) {
            // Sanifica la risposta finale per il JSON
            char sanitized_final[BUFFER_SIZE];
            sanitize_for_json(final_response, sanitized_final, sizeof(sanitized_final));
            
            char closingJson[BUFFER_SIZE];
            int written = snprintf(closingJson, sizeof(closingJson),
                "{ \"type\": \"gpt_closing\", \"message\": \"%s\" }\n", sanitized_final);

            if (written < 0 || written >= (int)sizeof(closingJson)) {
                fprintf(stderr, "Warning: closingJson troncato (%d caratteri)\n", written);
            }
            send(clientFileDescriptor, closingJson, strlen(closingJson), 0);
            LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent GPT closing message");
            LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", closingJson);
        } else {
            goto static_closing;
        }
    } else {
        static_closing:
        // Messaggio di chiusura statico se GPT non è disponibile
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
        LOG_CLIENT_INFO(clientAddress, "→ CLIENT: Sent STATIC closing message");
        LOG_CLIENT_DEBUG(clientAddress, "→ JSON: %s", closingJson);
    }

cleanup:
    // Cleanup sessione GPT
    if (gpt_session) {
        gpt_destroy_session(gpt_session);
        LOG_CLIENT_INFO(clientAddress, "Complete assessment and follow-up completed, GPT session destroyed");
    }
}

// Implementazione della funzione determine_behavior_style
const char* determine_behavior_style(double extroversion, int question_index, const char* previous_response) {
    // Per la prima domanda, usa sempre neutrale
    if (question_index == 0) {
        return "neutrale";
    }
    
    // Analizza la risposta precedente per determinare il comportamento appropriato
    if (!previous_response) {
        return "neutrale";
    }
    
    int response_value = atoi(previous_response);
    
    // Logica per determinare il comportamento basato su estroversione e risposta
    if (extroversion < 3.0) {
        // Persona tendenzialmente riservata
        if (response_value <= 3) {
            return "empatico";  // Mostra comprensione per risposte che indicano timidezza
        } else {
            return "curioso";   // Mostra interesse quando è più aperta del previsto
        }
    } else if (extroversion >= 5.0) {
        // Persona tendenzialmente aperta
        if (response_value >= 5) {
            return "entusiasta"; // Condivide l'entusiasmo
        } else {
            return "empatico";   // Mostra comprensione per aspetti meno estroversi
        }
    } else {
        // Persona neutrale - varia il comportamento in base alla domanda
        if (question_index % 3 == 0) {
            return "curioso";    // Mostra curiosità ogni 3 domande
        } else if (question_index % 3 == 1) {
            return "empatico";   // Mostra empatia
        } else {
            return "neutrale";   // Mantieni neutralità
        }
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

// Function to sanitize GPT response for JSON formatting
void sanitize_for_json(char *input, char *output, size_t output_size) {
    size_t input_len = strlen(input);
    size_t output_pos = 0;
    
    for (size_t i = 0; i < input_len && output_pos < output_size - 1; i++) {
        char c = input[i];
        
        // Replace newlines and carriage returns with spaces
        if (c == '\n' || c == '\r') {
            // Only add space if the previous character wasn't already a space
            if (output_pos > 0 && output[output_pos - 1] != ' ') {
                output[output_pos++] = ' ';
            }
        }
        // Escape quotes for JSON
        else if (c == '"') {
            if (output_pos < output_size - 2) {
                output[output_pos++] = '\\';
                output[output_pos++] = '"';
            }
        }
        // Escape backslashes for JSON
        else if (c == '\\') {
            if (output_pos < output_size - 2) {
                output[output_pos++] = '\\';
                output[output_pos++] = '\\';
            }
        }
        // Copy regular characters
        else {
            output[output_pos++] = c;
        }
    }
    
    output[output_pos] = '\0';
    
    // Trim trailing spaces
    while (output_pos > 0 && output[output_pos - 1] == ' ') {
        output[--output_pos] = '\0';
    }
}
