#ifndef BACKUP_SYNC_H
#define BACKUP_SYNC_H

#include "ss.h"

// Backup and synchronization functions
void create_backup_path(const char *filename, char *backup_file);
void sync_to_backup(const char *filename, const char *backup_file);
void sync_directory_to_backup(const char *dir, const char *backup_dir);
void duplicate_directory(const char *src_dir, const char *dest_dir);
void copy_file(const char *source, const char *destination);
int create_directory(const char *path);
void ss_duplicate(const char *storage_path);

#endif