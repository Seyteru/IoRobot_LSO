#include "gpt_client.h"
#include "logger.h"
#include <json-c/json.h>
#include <time.h>

// Variabili globali per la configurazione
static char g_api_key[256] = {0};
static char g_api_url[256] = {0};
static char g_model[64] = {0};
static int g_max_tokens = 150;
static float g_temperature = 0.7;

// Callback per ricevere dati da CURL
size_t gpt_write_callback(void *contents, size_t size, size_t nmemb, gpt_response_t *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->memory, response->size + realsize + 1);
    
    if (!ptr) {
        LOG_ERROR("Not enough memory for GPT response (realloc failed)");
        return 0;
    }
    
    response->memory = ptr;
    memcpy(&(response->memory[response->size]), contents, realsize);
    response->size += realsize;
    response->memory[response->size] = 0;
    
    return realsize;
}

// Carica la configurazione dal file .env
int gpt_load_config() {
    FILE *env_file = fopen(".env", "r");
    if (!env_file) {
        LOG_ERROR("Cannot open .env file");
        return -1;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), env_file)) {
        // Rimuovi newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Salta righe vuote e commenti
        if (line[0] == '\0' || line[0] == '#') continue;
        
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");
        
        if (!key || !value) continue;
        
        if (strcmp(key, "OPENAI_API_KEY") == 0) {
            strncpy(g_api_key, value, sizeof(g_api_key) - 1);
        } else if (strcmp(key, "OPENAI_API_URL") == 0) {
            strncpy(g_api_url, value, sizeof(g_api_url) - 1);
        } else if (strcmp(key, "OPENAI_MODEL") == 0) {
            strncpy(g_model, value, sizeof(g_model) - 1);
        } else if (strcmp(key, "OPENAI_MAX_TOKENS") == 0) {
            g_max_tokens = atoi(value);
        } else if (strcmp(key, "OPENAI_TEMPERATURE") == 0) {
            g_temperature = atof(value);
        }
    }
    
    fclose(env_file);
    
    if (strlen(g_api_key) == 0) {
        LOG_ERROR("OPENAI_API_KEY not found in .env file");
        return -1;
    }
    
    LOG_INFO("GPT configuration loaded successfully");
    return 0;
}

// Carica il template del prompt
char* gpt_load_prompt_template() {
    FILE *prompt_file = fopen("gpt_prompt.txt", "r");
    if (!prompt_file) {
        LOG_ERROR("Cannot open gpt_prompt.txt file");
        return NULL;
    }
    
    // Calcola la dimensione del file
    fseek(prompt_file, 0, SEEK_END);
    long file_size = ftell(prompt_file);
    fseek(prompt_file, 0, SEEK_SET);
    
    char *prompt = malloc(file_size + 1);
    if (!prompt) {
        LOG_ERROR("Cannot allocate memory for prompt template");
        fclose(prompt_file);
        return NULL;
    }
    
    size_t read = fread(prompt, 1, file_size, prompt_file);
    if (read != (size_t)file_size) {
        fprintf(stderr, "Error: letti solo %zu byte su %zu dal prompt\n", read, file_size);
    }
    prompt[file_size] = '\0';
    fclose(prompt_file);
    
    LOG_DEBUG("Prompt template loaded (%ld characters)", file_size);
    return prompt;
}

// Inizializzazione globale
int gpt_init_global() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        LOG_ERROR("Failed to initialize CURL globally");
        return -1;
    }
    
    if (gpt_load_config() != 0) {
        curl_global_cleanup();
        return -1;
    }
    
    LOG_INFO("GPT client initialized globally");
    return 0;
}

// Cleanup globale
void gpt_cleanup_global() {
    curl_global_cleanup();
    LOG_INFO("GPT client cleaned up globally");
}

// Crea una nuova sessione GPT
gpt_session_t* gpt_create_session() {
    gpt_session_t* session = malloc(sizeof(gpt_session_t));
    if (!session) {
        LOG_ERROR("Cannot allocate memory for GPT session");
        return NULL;
    }
    
    // Copia la configurazione globale
    snprintf(session->api_key, sizeof(session->api_key), "%s", g_api_key);
    snprintf(session->api_url, sizeof(session->api_url), "%s", g_api_url);
    snprintf(session->model, sizeof(session->model), "%s", g_model);
    session->max_tokens = g_max_tokens;
    session->temperature = g_temperature;
    
    session->message_count = 0;
    session->initialized = 0;
    
    // Genera un session ID unico
    snprintf(session->session_id, sizeof(session->session_id), "sess_%ld_%d", time(NULL), rand() % 10000);
    
    LOG_INFO("Created GPT session: %s", session->session_id);
    return session;
}

// Distrugge una sessione GPT
void gpt_destroy_session(gpt_session_t* session) {
    if (session) {
        LOG_INFO("Destroying GPT session: %s", session->session_id);
        free(session);
    }
}

// Inizializza la sessione con il prompt di sistema
int gpt_initialize_session(gpt_session_t* session, const char* system_prompt) {
    if (!session || !system_prompt) {
        LOG_ERROR("Invalid parameters for session initialization");
        return -1;
    }
    
    // Aggiungi il messaggio di sistema
    strcpy(session->messages[0].role, "system");
    strncpy(session->messages[0].content, system_prompt, sizeof(session->messages[0].content) - 1);
    session->message_count = 1;
    session->initialized = 1;
    
    LOG_INFO("GPT session %s initialized with system prompt", session->session_id);
    return 0;
}

// Formatta il prompt per la personalità
int gpt_format_personality_prompt(char* output, size_t output_size, 
                                  const char* user_response, 
                                  const char* behavior_style, 
                                  const char* next_question) {
    if (!output || !behavior_style || !next_question) {
        LOG_ERROR("Invalid parameters for prompt formatting");
        return -1;
    }
    
    const char* response_part = user_response ? user_response : "nullo";
    
    snprintf(output, output_size, "[%s]; [%s]; [%s]", 
             response_part, behavior_style, next_question);
    
    LOG_DEBUG("Formatted prompt: %s", output);
    return 0;
}

// Invia un messaggio e riceve la risposta
int gpt_send_message(gpt_session_t* session, const char* user_message, char* response, size_t response_size) {
    if (!session || !session->initialized || !user_message || !response) {
        LOG_ERROR("Invalid parameters for GPT message");
        return -1;
    }
    
    CURL *curl;
    CURLcode res;
    gpt_response_t gpt_response = {0};
    
    curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL for GPT request");
        return -1;
    }
    
    // Aggiungi il messaggio dell'utente alla conversazione
    if (session->message_count < MAX_CONVERSATION_HISTORY - 1) {
        strcpy(session->messages[session->message_count].role, "user");
        strncpy(session->messages[session->message_count].content, user_message, 
                sizeof(session->messages[session->message_count].content) - 1);
        session->message_count++;
    }
    
    // Costruisci il JSON per la richiesta
    json_object *json_request = json_object_new_object();
    json_object *json_model = json_object_new_string(session->model);
    json_object *json_max_tokens = json_object_new_int(session->max_tokens);
    json_object *json_temperature = json_object_new_double(session->temperature);
    json_object *json_messages = json_object_new_array();
    
    // Aggiungi tutti i messaggi
    for (int i = 0; i < session->message_count; i++) {
        json_object *json_message = json_object_new_object();
        json_object *json_role = json_object_new_string(session->messages[i].role);
        json_object *json_content = json_object_new_string(session->messages[i].content);
        
        json_object_object_add(json_message, "role", json_role);
        json_object_object_add(json_message, "content", json_content);
        json_object_array_add(json_messages, json_message);
    }
    
    json_object_object_add(json_request, "model", json_model);
    json_object_object_add(json_request, "messages", json_messages);
    json_object_object_add(json_request, "max_tokens", json_max_tokens);
    json_object_object_add(json_request, "temperature", json_temperature);
    
    const char *json_string = json_object_to_json_string(json_request);
    
    // Configura CURL
    struct curl_slist *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", session->api_key);
    
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    
    curl_easy_setopt(curl, CURLOPT_URL, session->api_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gpt_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&gpt_response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    // Esegui la richiesta
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("CURL request failed: %s", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        json_object_put(json_request);
        if (gpt_response.memory) free(gpt_response.memory);
        return -1;
    }
      // Parsa la risposta JSON
    json_object *json_response = json_tokener_parse(gpt_response.memory);    if (!json_response) {
        LOG_ERROR("Failed to parse GPT response JSON");
        // Removed raw GPT response logging for privacy
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        json_object_put(json_request);
        if (gpt_response.memory) free(gpt_response.memory);
        return -1;    }
    
    json_object *choices, *first_choice, *message, *content;
    if (json_object_object_get_ex(json_response, "choices", &choices) &&
        json_object_is_type(choices, json_type_array) &&
        json_object_array_length(choices) > 0) {
        
        first_choice = json_object_array_get_idx(choices, 0);
        if (json_object_object_get_ex(first_choice, "message", &message) &&
            json_object_object_get_ex(message, "content", &content)) {
            
            const char *response_text = json_object_get_string(content);
            strncpy(response, response_text, response_size - 1);
            response[response_size - 1] = '\0';
            
            // Aggiungi la risposta dell'assistente alla conversazione
            if (session->message_count < MAX_CONVERSATION_HISTORY - 1) {
                strcpy(session->messages[session->message_count].role, "assistant");
                strncpy(session->messages[session->message_count].content, response_text, 
                        sizeof(session->messages[session->message_count].content) - 1);
                session->message_count++;            }
            
            LOG_INFO("GPT response received (session: %s)", session->session_id);
            // Removed detailed GPT response logging for privacy
        } else {
            LOG_ERROR("Invalid GPT response structure");
            json_object_put(json_response);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            json_object_put(json_request);
            if (gpt_response.memory) free(gpt_response.memory);
            return -1;
        }
    } else {
        LOG_ERROR("No choices in GPT response");
        json_object_put(json_response);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        json_object_put(json_request);
        if (gpt_response.memory) free(gpt_response.memory);
        return -1;
    }
    
    // Cleanup
    json_object_put(json_response);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_object_put(json_request);
    if (gpt_response.memory) free(gpt_response.memory);
    
    return 0;
}
