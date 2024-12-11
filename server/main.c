#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(){
    printf ("Server starting...\n");

    if(initializeServer() != 0){
        fprintf(stderr, "Server Initialization Failure!\n");
        return EXIT_FAILURE;
    }

    runServer();

    shutdownServer();
    
    printf("Server Closed!\n");

    return 0;
}