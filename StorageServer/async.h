#ifndef ASYNC_H
#define ASYNC_H

#include "ss.h"

// Async write function
void *async_write(int client_socket, const char *filename, char *content, const char * client_ip);

// Async helper functions
void copy_async_file(const char *source, const char *destination);
void get_parent_directory(const char *filename, char *parent_directory);

#endif