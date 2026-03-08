#include "backup_utils.h"

// Helper function to remove prefix (from original ns.c)
void remove_prefix(char *str, const char *prefix) {
    char *pos = strstr(str, prefix);
    if (pos == str) { // Check if the prefix is at the beginning
        memmove(pos, pos + strlen(prefix), strlen(pos + strlen(prefix)) + 1);
    }
}

void initialize_backup_directory_network(int server_id, int n, const char *src_dir, const char *dest_dir1, const char *dest_dir2)
{
    int backup1 = (server_id + 1) % n;
    int backup2 = (server_id + 2) % n;

    char dest_dir_backup1[PATH_MAX], dest_dir_backup2[PATH_MAX];
    snprintf(dest_dir_backup1, PATH_MAX, "SS%d/backup_s%d_s%d", backup1 + 1, backup1, server_id);
    snprintf(dest_dir_backup2, PATH_MAX, "SS%d/backup_s%d_s%d", backup2 + 1, backup2, server_id);
    char src_dir_final[PATH_MAX];
    snprintf(src_dir_final, PATH_MAX, "SS%d/backup", server_id + 1);
    printf("%s\n", src_dir_final);

    printf("Initializing network backup directories for Server %d\n", server_id + 1);

    // Use network-based copy for backup initialization
    send_create(storage_servers[backup1].ip, storage_servers[backup1].port, dest_dir_backup1, 1);
    copy_directory_network(
        src_dir_final,
        dest_dir_backup1,
        storage_servers[server_id].ip,
        storage_servers[backup1].ip,
        storage_servers[server_id].port,
        storage_servers[backup1].port,
        0);
    sleep(10);
    insert_path(file_trie, dest_dir_backup1, storage_servers[backup1].ip, storage_servers[backup1].port);
    printf("%s\n", src_dir_final);
    send_create(storage_servers[backup2].ip, storage_servers[backup2].port, dest_dir_backup2, 1);
    copy_directory_network(
        src_dir_final,
        dest_dir_backup2,
        storage_servers[server_id].ip,
        storage_servers[backup2].ip,
        storage_servers[server_id].port,
        storage_servers[backup2].port,
        0);
    insert_path(file_trie, dest_dir_backup2, storage_servers[backup2].ip, storage_servers[backup2].port);
}