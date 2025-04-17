#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

int initializeServer();

void runServer();

void shutdownServer();

#endif