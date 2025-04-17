#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

extern LogLevel currentLogLevel;

void setLogLevel(LogLevel level);

void logMessage(LogLevel level, const char* file, int line, const char* format, ...);

void logClientMessage(LogLevel level, const char* file, int line, 
                     struct sockaddr_in* clientAddr, const char* format, ...);

#define LOG_DEBUG(...) logMessage(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) logMessage(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) logMessage(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logMessage(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_CLIENT_DEBUG(clientAddr, ...) logClientMessage(LOG_DEBUG, __FILE__, __LINE__, clientAddr, __VA_ARGS__)
#define LOG_CLIENT_INFO(clientAddr, ...) logClientMessage(LOG_INFO, __FILE__, __LINE__, clientAddr, __VA_ARGS__)
#define LOG_CLIENT_WARNING(clientAddr, ...) logClientMessage(LOG_WARNING, __FILE__, __LINE__, clientAddr, __VA_ARGS__)
#define LOG_CLIENT_ERROR(clientAddr, ...) logClientMessage(LOG_ERROR, __FILE__, __LINE__, clientAddr, __VA_ARGS__)

#endif