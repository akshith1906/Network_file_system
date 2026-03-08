#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "ss.h"

// Define the FileLock structure that's used for file locking
typedef struct FileLock
{
    char filename[256];
    pthread_rwlock_t lock;
    struct FileLock *next;
} FileLock;

// Define file_header structure that is used for file transfers
typedef struct file_header
{
    char filename[256];
    size_t filesize;
    int type; // 0 for directory, 1 for file
} file_header;

// File operations
void send_file_content(int client_socket, const char *filename);
void write_to_file(int client_socket, const char *filename, const char *content);
void send_file_info(int client_socket, const char *filename);
void stream_audio(int client_socket, const char *filename);
void send_directory_contents(int sock, const char *path, char* parent_dir, int send_content);
int receive_file(int sock, const char *dest_path);

// File locking functions
pthread_rwlock_t *get_file_lock(const char *filename);
void destroy_all_file_locks();

#endif