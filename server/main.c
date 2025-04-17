#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"
#include "logger.h"

void signalHandler(int sig) {
    LOG_INFO("Received signal %d, shutting down server...", sig);
    shutdownServer();
    exit(0);
}

int main(){
    setLogLevel(LOG_DEBUG);
    
    LOG_INFO("Server starting...");

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(initializeServer() != 0){
        LOG_ERROR("Server Initialization Failure!");
        return EXIT_FAILURE;
    }

    runServer();

    shutdownServer();
    
    LOG_INFO("Server Closed!");

    return 0;
}