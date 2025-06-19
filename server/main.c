#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"
#include "logger.h"
#include "gpt_client.h"

// Funzione di test leggero per verificare la configurazione GPT (senza consumare token)
int test_gpt_configuration() {
    LOG_INFO("Testing GPT configuration...");
    
    // Crea una sessione di test
    gpt_session_t* test_session = gpt_create_session();
    if (!test_session) {
        LOG_ERROR("Failed to create GPT test session");
        return -1;
    }
    
    // Verifica che la configurazione sia valida
    if (strlen(test_session->api_key) == 0) {
        LOG_ERROR("GPT API key not configured");
        gpt_destroy_session(test_session);
        return -1;
    }
    
    if (strlen(test_session->api_url) == 0) {
        LOG_ERROR("GPT API URL not configured");
        gpt_destroy_session(test_session);
        return -1;
    }
    
    if (strlen(test_session->model) == 0) {
        LOG_ERROR("GPT model not configured");
        gpt_destroy_session(test_session);
        return -1;
    }
    
    // Verifica che il template del prompt esista
    char* system_prompt = gpt_load_prompt_template();
    if (!system_prompt) {
        LOG_ERROR("Failed to load GPT prompt template");
        gpt_destroy_session(test_session);
        return -1;
    }
    free(system_prompt);
    
    LOG_INFO("=== GPT CONFIGURATION VERIFIED ===");
    LOG_INFO("API Key: %.*s... (configured)", 10, test_session->api_key);
    LOG_INFO("Model: %s", test_session->model);
    LOG_INFO("Max Tokens: %d", test_session->max_tokens);
    LOG_INFO("Temperature: %.1f", test_session->temperature);
    LOG_INFO("==================================");
    
    // Cleanup
    gpt_destroy_session(test_session);
    return 0;
}

void signalHandler(int sig) {
    LOG_INFO("Received signal %d, shutting down server...", sig);
    gpt_cleanup_global();
    shutdownServer();
    exit(0);
}

int main(){
    setLogLevel(LOG_DEBUG);
    
    LOG_INFO("IoRobot Personality Assessment Server starting...");
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Inizializza il sistema GPT globalmente
    if (gpt_init_global() != 0) {
        LOG_ERROR("Failed to initialize GPT system");
        return EXIT_FAILURE;
    }

    // Test configurazione GPT (senza consumare token)
    if (test_gpt_configuration() != 0) {
        LOG_WARNING("GPT configuration test failed - server will run with fallback mode only");
    } else {
        LOG_INFO("GPT system ready - dynamic question generation enabled");
    }

    // Inizializza il server
    if(initializeServer() != 0){
        LOG_ERROR("Server Initialization Failure!");
        gpt_cleanup_global();
        return EXIT_FAILURE;
    }

    LOG_INFO("Server ready - waiting for connections...");
    runServer();

    // Cleanup
    shutdownServer();
    gpt_cleanup_global();
    
    LOG_INFO("Server Closed!");

    return 0;
}