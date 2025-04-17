#include "logger.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

LogLevel currentLogLevel = LOG_INFO;

static const char* logLevelNames[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

// Colors for log levels (ANSI escape sequences)
static const char* logLevelColors[] = {
    "\033[36m", // Cyan for DEBUG
    "\033[32m", // Green for INFO
    "\033[33m", // Yellow for WARNING
    "\033[31m"  // Red for ERROR
};

static const char* resetColor = "\033[0m";

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

void setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

void logMessage(LogLevel level, const char* file, int line, const char* format, ...) {
    if (level < currentLogLevel) {
        return;
    }

    pthread_mutex_lock(&logMutex);

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    pthread_t tid = pthread_self();
    
    const char* basename = strrchr(file, '/');
    if (basename) {
        basename++;
    } else {
        basename = file;
    }
    
    fprintf(stderr, "%s%s [%s] [Thread %lu] [%s:%d] %s", 
            logLevelColors[level], timestamp, logLevelNames[level], 
            (unsigned long)tid, basename, line, resetColor);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    fflush(stderr);
    
    pthread_mutex_unlock(&logMutex);
}

void logClientMessage(LogLevel level, const char* file, int line, 
                     struct sockaddr_in* clientAddr, const char* format, ...) {
    if (level < currentLogLevel) {
        return;
    }

    pthread_mutex_lock(&logMutex);

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    pthread_t tid = pthread_self();
    
    const char* basename = strrchr(file, '/');
    if (basename) {
        basename++;
    } else {
        basename = file;
    }
    
    char clientInfo[50] = "Unknown client";
    if (clientAddr) {
        snprintf(clientInfo, sizeof(clientInfo), "%s:%d", 
                 inet_ntoa(clientAddr->sin_addr), 
                 ntohs(clientAddr->sin_port));
    }
    
    fprintf(stderr, "%s%s [%s] [Thread %lu] [%s:%d] [Client %s] %s", 
            logLevelColors[level], timestamp, logLevelNames[level], 
            (unsigned long)tid, basename, line, clientInfo, resetColor);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    fflush(stderr);
    
    pthread_mutex_unlock(&logMutex);
}