#ifndef LOG_H
#define LOG_H

#include "ss.h"

// Log function declarations
void log_message(int level, const char* format, ...);
void cleanup();

// Log levels
#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 2

#endif