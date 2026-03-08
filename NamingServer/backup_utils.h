#ifndef BACKUP_UTILS_H
#define BACKUP_UTILS_H

#include "ns.h"

// Function declaration for backup initialization
void initialize_backup_directory_network(int server_id, int n, const char *src_dir, const char *dest_dir1, const char *dest_dir2);
void remove_prefix(char *str, const char *prefix);

#endif