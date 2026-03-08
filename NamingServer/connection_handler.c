#include "connection_handler.h"
#include "backup_utils.h"

// Helper function to remove prefix (from original ns.c) - but this will be declared in backup_utils.h
// void remove_prefix(char *str, const char *prefix) {
//     char *pos = strstr(str, prefix);
//     if (pos == str) { // Check if the prefix is at the beginning
//         memmove(pos, pos + strlen(prefix), strlen(pos + strlen(prefix)) + 1);
//     }
// }

void *handle_client(void *client_socket)
{
    int sock = *(int *)client_socket;
    char buffer[BUFFER_SIZE] = {0};

    // Provide the first available storage server to the client
    pthread_mutex_lock(&mutex);
    if (server_count > 0)
    {
        snprintf(buffer, sizeof(buffer), "%s:%d", storage_servers[MAX_STORAGE_SERVERS - 1].ip, storage_servers[MAX_STORAGE_SERVERS - 1].port);
    }
    else
    {
        strcpy(buffer, "No storage servers available");
    }
    pthread_mutex_unlock(&mutex);

    send(sock, buffer, strlen(buffer), 0);
    printf("Sent storage server info to client: %s\n", buffer);

    close(sock);
    return NULL;
}

void *handle_ss_registration(void *client_socket_ptr)
{
    int client_socket = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    // Get client address information
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    getpeername(client_socket, (struct sockaddr *)&peer_addr, &peer_addr_len);

    char buffer[BUFFER_SIZE];
    char server_ip[INET_ADDRSTRLEN];
    int server_port;

    // fflush(buffer);
    // pthread_mutex_lock(&mutex);
    // printf("Before Receive: %s", buffer);
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    // printf("Received After: %s\n", buffer);
    // pthread_mutex_unlock(&mutex);

    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        log_message(LOG_INFO, "Received registration request from %s:%d - Data: %s",
                    inet_ntoa(peer_addr.sin_addr),
                    ntohs(peer_addr.sin_port),
                    buffer);

        // Parse the received IP and port
        if (sscanf(buffer, "%[^:]:%d", server_ip, &server_port) != 2)
        {
            log_message(LOG_ERROR, "Failed to parse IP:Port from storage server data: %s", buffer);
            close(client_socket);
            return NULL;
        }
        bool flag1 = register_server(server_ip, server_port);
        if (flag1 == true)
        {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        int flag = 0;
        for (int i = 0; i < server_count; i++)
        {
            if (storage_servers[i].port == server_port && !strcmp(storage_servers[i].ip, server_ip))
            {
                flag = 1;
            }
        }
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            log_message(LOG_ERROR, "Error receiving directory path from %s:%d",
                        server_ip, server_port);
            close(client_socket);
            return NULL;
        }
        buffer[bytes_received] = '\0';

        char directory_path[BUFFER_SIZE];
        strncpy(directory_path, buffer, BUFFER_SIZE);
        log_message(LOG_INFO, "Storage Server %s:%d - Directory Path: %s",
                    server_ip, server_port, directory_path);
        insert_path(file_trie, buffer, server_ip, server_port);

        // Step 3: Receive the file structure from the Storage Server
        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
        {
            buffer[bytes_received] = '\0';

            // Check if it's the end of the file list
            if (strcmp(buffer, "END") == 0)
            {
                break;
            }

            log_message(LOG_INFO, "Received file: %s", buffer);
            insert_path(file_trie, buffer, server_ip, server_port);
        }
        print_all_paths(file_trie);
        if (!flag)
        {
            // Add storage server to the list with proper synchronization
            pthread_mutex_lock(&mutex);

            if (server_count < MAX_STORAGE_SERVERS)
            {
                // Add to array

                strcpy(storage_servers[server_count].ip, server_ip);
                storage_servers[server_count].port = server_port;
                init_server_health(server_count, server_ip, server_port);
                strncpy(src_dest[server_count].dir_path, directory_path, BUFFER_SIZE);
                server_count++;

                // Create new server node
                StorageServernew *new_server = malloc(sizeof(StorageServernew));
                if (!new_server)
                {
                    log_message(LOG_ERROR, "Failed to allocate memory for StorageServer at %s:%d",
                                server_ip, server_port);
                    pthread_mutex_unlock(&mutex);
                    close(client_socket);
                    return NULL;
                }

                // Initialize new server
                strcpy(new_server->ip, server_ip);
                new_server->port = server_port;
                new_server->file_structure = create_node();

                // Uncomment and modify if path processing is needed
                /*
                char *path = strtok(paths, ",");
                while (path) {
                    insert_path(new_server->file_structure, path);
                    path = strtok(NULL, ",");
                }
                new_server->next = server_list;
                server_list = new_server;
                */

                log_message(LOG_INFO, "Successfully registered storage server %s:%d (Server Count: %d)",
                            server_ip, server_port, server_count);
            }
            else
            {
                log_message(LOG_WARNING, "Maximum storage server limit reached. Rejected server %s:%d",
                            server_ip, server_port);
            }

            pthread_mutex_unlock(&mutex);

            if (server_count == 3)
            { // major urgent: add path to trie and delete it from trie
                for (int i = 0; i < 3; i++)
                {
                    initialize_backup_directory_network(i, server_count, src_dest[i % server_count].dir_path, src_dest[(i + 1) % server_count].dir_path, src_dest[(i + 2) % server_count].dir_path);
                    sleep(10);
                }
            }
            else if (server_count > 3)
            {
                initialize_backup_directory_network(server_count - 1, server_count, src_dest[(server_count - 1) % server_count].dir_path, src_dest[(server_count) % server_count].dir_path, src_dest[(server_count + 1) % server_count].dir_path);
                sleep(5);
                initialize_backup_directory_network(server_count - 2, server_count, src_dest[(server_count - 2) % server_count].dir_path, src_dest[(server_count - 1) % server_count].dir_path, src_dest[(server_count) % server_count].dir_path);
                sleep(5);
                initialize_backup_directory_network(server_count - 3, server_count, src_dest[(server_count - 3) % server_count].dir_path, src_dest[(server_count - 2) % server_count].dir_path, src_dest[(server_count - 1) % server_count].dir_path);
                // sleep(5);
                char del_dir_path1[PATH_MAX];
                char del_dir_path2[PATH_MAX];
                snprintf(del_dir_path1, PATH_MAX, "SS1/backup_s0_s%d", server_count - 3);
                snprintf(del_dir_path2, PATH_MAX, "SS2/backup_s1_s%d", server_count - 2);
                send_delete(storage_servers[0].ip, storage_servers[0].port, del_dir_path1, 1);
                send_delete(storage_servers[1].ip, storage_servers[1].port, del_dir_path2, 1);
                delete_path(file_trie, del_dir_path1, storage_servers[0].ip, storage_servers[0].port);
                delete_path(file_trie, del_dir_path2, storage_servers[1].ip, storage_servers[1].port);

                // del backup_s2_s%d(server_count-1)
                // del backup_s1_s%d(server_count-2)
            }
        }
    }
    else if (bytes_received == 0)
    {
        log_message(LOG_WARNING, "Connection closed by Storage Server at %s:%d",
                    inet_ntoa(peer_addr.sin_addr),
                    ntohs(peer_addr.sin_port));
    }
    else
    {
        log_message(LOG_ERROR, "Error receiving data from %s:%d - %s",
                    inet_ntoa(peer_addr.sin_addr),
                    ntohs(peer_addr.sin_port),
                    strerror(errno));
    }

    close(client_socket);
    return NULL;
}