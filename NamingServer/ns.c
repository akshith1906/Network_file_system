#include "ns.h"
#include "connection_handler.h"
#include "health_monitor.h"
#include "command_processor.h"
#include "backup_utils.h"

// Global variables
typedef struct FileNode
{
    char path[BUFFER_SIZE];
    struct FileNode *next;
} FileNode;

serdest src_dest[MAX_STORAGE_SERVERS];

TrieNode *file_trie;
LRUCache *cache;
int async_writing[MAX_STORAGE_SERVERS] = {0}; // Initialize array to 0

// Function to insert a file path into the file list
FileNode *create_file_node(const char *path)
{
    FileNode *new_node = (FileNode *)malloc(sizeof(FileNode));
    if (new_node)
    {
        strncpy(new_node->path, path, BUFFER_SIZE);
        new_node->next = NULL;
    }
    return new_node;
}

void insert_file(FileNode **head, const char *path)
{
    FileNode *new_node = create_file_node(path);
    if (new_node)
    {
        new_node->next = *head;
        *head = new_node;
    }
}




// Global variables that were defined in original ns.c
StorageServer storage_servers[MAX_STORAGE_SERVERS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t health_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_count = 0;
ServerHealth server_health[MAX_STORAGE_SERVERS];

// The register_server function was originally in the main ns.c file
bool register_server(const char *ip,int port){
    for(int i=0;i<server_count;i++){
        printf("port=%d",server_health[i].port);
        if(server_health[i].port==port){
            server_health[i].is_active=true;
            printf("server with port %d back online\n",port);
            printf("%d %d\n",server_count,i);
            if(server_count>=3){
                sleep(10);
                char src_path1[MAX_PATH_LENGTH],src_path2[MAX_PATH_LENGTH];
                snprintf(src_path1,MAX_PATH_LENGTH,"%s/backup",src_dest[(i-1+server_count)%server_count].dir_path);
                snprintf(src_path2,MAX_PATH_LENGTH,"%s/backup",src_dest[(i-2+server_count)%server_count].dir_path);

                char dest_path1[MAX_PATH_LENGTH],dest_path2[MAX_PATH_LENGTH];
                snprintf(dest_path1,MAX_PATH_LENGTH,"%s/backup_s%d_s%d",src_dest[i].dir_path,i,(i-1+server_count)%server_count);
                snprintf(dest_path2,MAX_PATH_LENGTH,"%s/backup_s%d_s%d",src_dest[i].dir_path,i,(i-2+server_count)%server_count);

                remove_prefix(src_path1,"Test/");
                remove_prefix(src_path2,"Test/");
                remove_prefix(dest_path1,"Test/");
                remove_prefix(dest_path2,"Test/");

                copy_directory_network(src_path1,dest_path1,storage_servers[(i-1+server_count)%server_count].ip,storage_servers[i].ip,storage_servers[(i-1+server_count)%server_count].port,storage_servers[i].port,0);
                sleep(5);
                printf("works\n");
                copy_directory_network(src_path2,dest_path2,storage_servers[(i-2+server_count)%server_count].ip,storage_servers[i].ip,storage_servers[(i-2+server_count)%server_count].port,storage_servers[i].port,0);

                char src_path3[MAX_PATH_LENGTH]; // for the server coming up
                snprintf(src_path3,MAX_PATH_LENGTH,"%s/backup",src_dest[i].dir_path);
                char dest_path3[MAX_PATH_LENGTH], dest_path4[MAX_PATH_LENGTH]; // for backups coming up
                snprintf(dest_path3,MAX_PATH_LENGTH,"%s/backup_s%d_s%d",src_dest[(i+1)%server_count].dir_path,(i+1)%server_count,i);
                snprintf(dest_path4,MAX_PATH_LENGTH,"%s/backup_s%d_s%d",src_dest[(i+2)%server_count].dir_path,(i+2)%server_count,i);
                remove_prefix(src_path3,"Test/");
                remove_prefix(dest_path3,"Test/");
                remove_prefix(dest_path4,"Test/");

                copy_directory_network(src_path3,dest_path3,storage_servers[i].ip,storage_servers[(i+1)%server_count].ip,storage_servers[i].port,storage_servers[(i+1)%server_count].port,0);
                sleep(5);
                printf("works\n");
                copy_directory_network(src_path3,dest_path4,storage_servers[i].ip,storage_servers[(i+2)%server_count].ip,storage_servers[i].port,storage_servers[(i+2)%server_count].port,0);

            }
            return true;
        }
    }
    return false;
}


int main()
{
    // Register cleanup function
    atexit(cleanup);
    file_trie = create_node();
    int server_fd, nm_socket, new_socket, client_socket;
    struct sockaddr_in address, nm_address, client_address;
    socklen_t addrlen = sizeof(address);
    socklen_t client_addr_len = sizeof(client_address);
    int opt = 1;

    // Initialize LRU Cache
    cache = create_lru_cache(10); // Cache size of 10 entries

    // Create socket for the naming server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Naming server socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("Failed to set SO_REUSEADDR");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configure naming server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(NM_PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 5) == -1)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Naming server is running on port %d...\n", NM_PORT);

    pthread_t health_thread;
    if (pthread_create(&health_thread, NULL, health_monitor, NULL) != 0)
    {
        log_message(LOG_ERROR, "Failed to create health monitoring thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(health_thread);

    // Accept and process connections
    while (1)
    {
        char temp_buffer[PATH_MAX];
        memset(temp_buffer, 0, sizeof(temp_buffer));
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == -1)
        {
            perror("Accept failed");
            continue;
        }

        // Check if the connection is from a client or a storage server
        pthread_t thread_id;
        // pthread_mutex_lock(&mutex);
        int bytes_peeked = recv(new_socket, temp_buffer, sizeof(temp_buffer), 0);
        printf("%d\n", bytes_peeked);
        temp_buffer[bytes_peeked] = '\0';
        char ssip[20];
        char reci[100];
        int ssport;
        int client;
        char temp[15];
        int ind = 0;
        printf("Temp Buffer: %s\n", temp_buffer);
        if (strncmp(temp_buffer, "Write completed", 15) != 0 && strncmp(temp_buffer, "ASYNC", 5) != 0)
        {
            printf("Temp Buffer: %s\n", temp_buffer);
        }
        else if (strncmp(temp_buffer, "Write completed", 15) == 0)
        {
            // printf("%s", temp_buffer);
            sscanf(temp_buffer, "Write completed at %s %d", reci, &client);
            printf("temp: %s", temp_buffer);
            printf("reci %s", reci);
            int flag = 0;
            for (int i = 0; i < strlen(reci); i++)
            {
                if (reci[i] == ':')
                {
                    flag = 1;
                }
                if (flag == 0)
                {
                    ssip[ind++] = reci[i];
                }
                if (flag == 2)
                {
                    temp[ind++] = reci[i];
                }
                if (flag == 1)
                {
                    ssip[ind] = '\0';
                    ind = 0;
                    flag = 2;
                }
            }
            temp[ind] = '\0';
            ssport = atoi(temp);

            // Create socket for health check
            int ss_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (ss_sock < 0)
            {
                perror("Socket creation failed");
                close(new_socket);
                exit(EXIT_FAILURE);
            }

            struct sockaddr_in server_address;
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(ssport); // Convert port to network byte order
            if (inet_pton(AF_INET, ssip, &server_address.sin_addr) <= 0)
            {
                perror("Invalid IP address");
                close(ss_sock);
                exit(EXIT_FAILURE);
            }

            // Connect to the server
            if (connect(ss_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
            {
                perror("Connection failed");
                close(ss_sock);
                exit(EXIT_FAILURE);
            }
            int health = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (storage_servers[i].port == ssport && !strcmp(storage_servers[i].ip, ssip))
                    health = 1;
            }
            if (!health)
            {
                send(ss_sock, "Unhealthy", 9, 0);
            }
            else
            {
                send(new_socket, "Healthy", 7, 0);
                // write(new_socket, "Write completed\n", 17);
            }
            continue;
        }
        else if (strncmp(temp_buffer, "ASYNC", 5) == 0)
        {
            if (strlen(temp_buffer) > 20)
            {
            }
            continue;
        }

        // pthread_mutex_unlock(&mutex);

        char command_buffer[100];
        strncpy(command_buffer, temp_buffer, sizeof(command_buffer) - 1);
        command_buffer[sizeof(command_buffer) - 1] = '\0';

        char *token = strtok(temp_buffer, " ");
        if (token != NULL)
        {
            if (strcmp(token, "STORAGE") == 0)
            {
                // Send acknowledgment to storage server
                char ack_msg[] = "ACK";
                send(new_socket, ack_msg, strlen(ack_msg), 0);

                // Handle storage server registration
                int *new_socket_ptr = malloc(sizeof(int));
                if (!new_socket_ptr)
                {
                    perror("Failed to allocate memory for socket pointer");
                    close(new_socket);
                    continue;
                }
                *new_socket_ptr = new_socket;
                if (pthread_create(&thread_id, NULL, handle_ss_registration, new_socket_ptr) != 0)
                {
                    perror("Failed to create thread for storage server");
                    free(new_socket_ptr);
                    close(new_socket);
                    continue;
                }
                pthread_detach(thread_id);
            }
            else if (strcmp(token, "CREATE") == 0 || strcmp(token, "COPY") == 0 || strcmp(token, "DELETE") == 0 || strcmp(token, "LIST") == 0)
            {
                handle_ns_commands(command_buffer);
                if (strcmp(token, "LIST") == 0) {
                    int saved_stdout = dup(STDOUT_FILENO);
                    dup2(new_socket, STDOUT_FILENO);
                    print_all_paths(file_trie);
                    dup2(saved_stdout, STDOUT_FILENO);
                    close(saved_stdout);
                    send(new_socket, "END", strlen("END"), 0);
                }
            }
            else
            {
                // Handle client requests
                char *command = strtok(command_buffer, " ");
                char *path = strtok(NULL, " ");
                printf("Command: %s, Path: %s\n", command, path);
                StorageServer *found_server = find_storage_server(file_trie, path);
                int *client_socket_ptr = malloc(sizeof(int));
                if (found_server == NULL) {
                    const char *error_msg = "Error: Storage server not found for the given path";
                    send(new_socket, error_msg, strlen(error_msg), 0);
                    close(new_socket);
                    free(client_socket_ptr);
                    continue;
                }
                storage_servers[MAX_STORAGE_SERVERS - 1] = *find_storage_server(file_trie, path);
                if (!client_socket_ptr)
                {
                    perror("Failed to allocate memory for socket pointer");
                    close(new_socket);
                    continue;
                }
                *client_socket_ptr = new_socket;
                if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0)
                {
                    perror("Failed to create thread for client");
                    free(client_socket_ptr);
                    close(new_socket);
                    continue;
                }
                pthread_detach(thread_id);
            }
        }
    }

    close(server_fd);
    return 0;
}