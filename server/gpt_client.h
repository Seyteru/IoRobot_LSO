#ifndef GPT_CLIENT_H
#define GPT_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define GPT_BUFFER_SIZE 4096
#define GPT_RESPONSE_SIZE 2048
#define MAX_CONVERSATION_HISTORY 50
#define MAX_MESSAGE_CONTENT 4096 

typedef struct {
    char *memory;
    size_t size;
} gpt_response_t;

typedef struct {
    char role[16];      // "system", "user", "assistant"
    char content[MAX_MESSAGE_CONTENT]; 
} conversation_message_t;

typedef struct {
    char api_key[256];
    char api_url[256];
    char model[64];
    int max_tokens;
    float temperature;
    
    conversation_message_t messages[MAX_CONVERSATION_HISTORY];
    int message_count;
    char session_id[64];
    int initialized;
} gpt_session_t;

// Funzioni principali
int gpt_init_global();
void gpt_cleanup_global();

gpt_session_t* gpt_create_session();
void gpt_destroy_session(gpt_session_t* session);

int gpt_initialize_session(gpt_session_t* session, const char* system_prompt);
int gpt_send_message(gpt_session_t* session, const char* user_message, char* response, size_t response_size);

// Funzioni di utilità
int gpt_load_config();
char* gpt_load_prompt_template();
int gpt_format_personality_prompt(char* output, size_t output_size, 
                                  const char* user_response, 
                                  const char* behavior_style, 
                                  const char* next_question);

// Callback per CURL
size_t gpt_write_callback(void *contents, size_t size, size_t nmemb, gpt_response_t *response);

#endif
