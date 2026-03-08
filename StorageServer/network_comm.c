#include "network_comm.h"
#include "backup_sync.h"
#include "file_operations.h"
#include "async.h"

extern int PORT;

void *handle_client(void *client_socket_ptr)
{
    int client_socket = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    buffer[bytes_read] = '\0';
    printf("%s\n", buffer);

    // Parse command and filename
    char command[256], filename[256], content[BUFFER_SIZE], flag[256], backup_file[300];

    sscanf(buffer, "%s %s %s %[^\n]", command, filename, content, flag); // fail

    printf("%s\n", command);
    printf("%s\n", filename);
    printf("%s\n",content);
    
    // Create backup path for this file
    create_backup_path(filename, backup_file);
    printf("%s\n", backup_file);

    if (strcmp(command, "READ") == 0)
    {
        pthread_rwlock_t *file_lock = get_file_lock(filename);
        if (!file_lock)
        {
            write(client_socket, "Failed to acquire file lock\n", 27);
            return NULL;
        }

        // Acquire read lock
        if (pthread_rwlock_rdlock(file_lock) != 0)
        {
            perror("Failed to acquire read lock");
            write(client_socket, "Lock Error\n", 11);
            return NULL;
        }

        // Perform the read operation
        send_file_content(client_socket, filename);

        // Release the lock
        if (pthread_rwlock_unlock(file_lock) != 0)
        {
            perror("Failed to release read lock");
        }
    }

    else if (strcmp(command, "WRITE") == 0)
    {
        pthread_rwlock_t *file_lock = get_file_lock(filename);
        if (!file_lock)
        {
            write(client_socket, "Failed to acquire file lock\n", 27);
            return NULL;
        }

        // Acquire write lock
        if (pthread_rwlock_wrlock(file_lock) != 0)
        {
            perror("Failed to acquire write lock");
            write(client_socket, "Lock Error\n", 11);
            return NULL;
        }

        // Perform the write operation
        if (strcmp(flag, "SYNC") == 0 || strlen(content) < 50)
        {
            write_to_file(client_socket, filename, content);
        }
        else
        {
            async_write(client_socket, filename, content, NM_IP);
        }
        sync_to_backup(filename,backup_file);

        // Release the lock
        if (pthread_rwlock_unlock(file_lock) != 0)
        {
            perror("Failed to release write lock");
        }
    }
    else if (strcmp(command, "INFO") == 0)
    {
        if(filename == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        send_file_info(client_socket, filename);
    }
    else if (strcmp(command, "STREAM") == 0)
    {
        if(filename == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        stream_audio(client_socket, filename);
    }
    else if (strcmp(command, "COPY_FILE") == 0)
    {
        create_backup_path(content,backup_file);
        if(filename == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        if (strcmp(filename, "DEST") == 0)
        {
            printf("Destination Port, receiving file from server at point: %s\n", content);
            receive_file(client_socket, content);
            if (strstr(content, "backup") != NULL) {
                // The string contains "backup", skip it
            } else {
                printf("content=%s\n", content);
                sync_to_backup(content, backup_file);
            }
        }
        else if (strcmp(filename, "SRC") == 0)
        {
            printf("Source Port, sending file from server at point: %s\n", content);
            send_file_content(client_socket, content);
        }
        printf("COPY FILE correctly parsed on Server\n");
    }
    else if (strcmp(command, "COPY_DIR") == 0)
    {
        create_backup_path(content,backup_file);
        if(filename == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        if (strcmp(filename, "DEST") == 0)
        {
            printf("Destination Port, receiving file from server at point: %s\n", content);
            char current_dir[BUFFER_SIZE];
            if (getcwd(current_dir, sizeof(current_dir)) == NULL)
            {
                perror("Failed to get current directory");
                return NULL;
            }
            receive_file(client_socket, content); // Change directory to the content path
            if (chdir(content) == -1)
            {
                perror("Failed to change directory to content path");
                return NULL;
            }

            char *flag_new = strdup(flag);
            char *flag_n = basename(flag_new);

            printf("%s\n", flag_new);

            // Extract the tar archive
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "tar -xzf %s.tar.gz", flag_n);
            int result = system(command);
            if (result == -1)
            {
                perror("Failed to extract tar archive");
                return NULL;
            }
            char tar_filepath[BUFFER_SIZE];
            snprintf(tar_filepath, sizeof(tar_filepath), "%s.tar.gz", flag_n);
            remove(tar_filepath);
            // Change back to the original directory
            if (chdir(current_dir) == -1)
            {
                perror("Failed to change back to the original directory");
                return NULL;
            }
            if (strstr(content, "backup") != NULL) {
                // The string contains "backup", skip it
            } else {
                printf("content=%s\n", content);
                sync_to_backup(content, backup_file);
            }
        }
        else if (strcmp(filename, "SRC") == 0)
        {
            printf("Source Port, sending file from server at point: %s\n", content);
            // Tar the directory
            char tar_command[BUFFER_SIZE];
            snprintf(tar_command, sizeof(tar_command), "tar -czf %s.tar.gz %s", content, content);
            int tar_result = system(tar_command);
            if (tar_result == -1)
            {
                perror("Failed to tar directory");
                return NULL;
            }

            // Send the tarred file
            char tar_filepath[BUFFER_SIZE];
            snprintf(tar_filepath, sizeof(tar_filepath), "%s.tar.gz", content);
            send_file_content(client_socket, tar_filepath);

            // Remove the tarred file after sending
            remove(tar_filepath);
            printf("COPY DIRECTORY correctly parsed on Server\n");
        }
        else
        {
            write(client_socket, "Invalid Command\n", 16);
        }
    }
    else if (strcmp("CREATE", command) == 0)
    {
        if(filename == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        if(content == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        int type = atoi(content);
        if (type == 0)
        {
            int fd = open(filename, O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
                perror("File not created");
            }
            else
            {
                printf("File created");
                int fx = open(backup_file, O_CREAT | O_TRUNC, 0644);
                if (fx < 0)
                {
                    perror("File not created");
                }
                else
                {
                    printf("File created");
                }
            }
        }
        else if (type == 1)
        {
            if (mkdir(filename, 0755) == 0)
            {
                printf("Directory created");
                if(strstr(filename, "backup") != NULL){

                }else{

                    if (mkdir(backup_file, 0755) == 0)
                    {
                        printf("Directory created");
                    }
                    else
                    {
                        perror("Directory not created");
                    }
                }
            }
            else
            {
                perror("Directory not created");
            }
        }
    }
    else if (strcmp(command, "DELETE") == 0)
    {
        if(filename == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        if(content == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return NULL;
        }
        int type = atoi(content);
        if (type == 0)
        {
            if (unlink(filename) == 0)
            {
                printf("File deleted");
                if (unlink(backup_file) == 0)
                {
                    printf("backup File deleted");
                }
                else
                {
                    perror("backup File not deleted");
                }
            }
            else
            {
                perror("File not deleted");
            }
            return 0;
        }
        if (type == 1)
        {
            char *args[] = {"rm", "-r", (char *)filename, NULL};
            char *backup_args[] = {"rm", "-r", (char *)backup_file, NULL};

            // Fork a new process to execute the command
            pid_t pid = fork();
            if (pid == -1)
            {
                perror("Failed to fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // Child process: execute the rm command
                execvp("rm", args);
                // If execvp fails, print an error and exit
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
            else
            {
                // Parent process: wait for the child process to complete
                int status;
                if (waitpid(pid, &status, 0) == -1)
                {
                    perror("waitpid failed");
                }
                else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    printf("Directory '%s' deleted successfully.\n", filename);
                }
                else
                {
                    printf("Failed to delete directory '%s'.\n", filename);
                }
            }
            pid = fork();
                if (pid == -1)
                {
                    perror("Failed to fork for backup");
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                {
                    execvp("rm", backup_args);
                    perror("execvp failed for backup");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    waitpid(pid, NULL, 0);
                }
            return 0;
        }
        return 0;
    }
    else if (strcmp(command, "LIST") == 0)
    {
        // list_directory(filename);
    }

    return NULL;
}

void register_with_naming_server(int PORT, const char *storage_path)
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(NM_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(NM_IP);

    // Connect to the naming server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        char err_mess[50];
        get_error_message(10, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        perror("Connection to naming server failed");
        close(sock);
        return;
    }

    // Step 1: Send initial command to register as a storage server
    char *initial_command = "STORAGE";
    send(sock, initial_command, strlen(initial_command), 0);
    printf("Command sent to Naming Server: %s\n", initial_command);

    // Step 2: Receive acknowledgment from Naming Server
    int ack_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (ack_received == -1)
    {
        char err_mess[50];
        get_error_message(9, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        perror("Failed to receive acknowledgment");
        close(sock);
        exit(EXIT_FAILURE);
    }
    buffer[ack_received] = '\0';

    if (strcmp(buffer, "ACK") != 0)
    {
        char err_mess[50];
        get_error_message(9, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        fprintf(stderr, "Did not receive ACK from Naming Server. Received: %s\n", buffer);
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Acknowledgment received from Naming Server: %s\n", buffer);

    // Step 3: Get the Storage Server's IP address
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        close(sock);
        return;
    }

    // Loop through linked list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        // Check for IPv4 address
        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                perror("getnameinfo");
                close(sock);
                freeifaddrs(ifaddr);
                return;
            }

            // Skip loopback address
            if (strcmp(host, "127.0.0.1") != 0)
            {
                IPbuffer = strdup(host);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);

    // Step 4: Send IP address and PORT to Naming Server
    snprintf(buffer, sizeof(buffer), "%s:%d", IPbuffer, PORT);
    send(sock, buffer, strlen(buffer), 0);
    printf("Sent IP and port to Naming Server: %s\n", buffer);

    // Step 5: Send the storage path to Naming Server
    usleep(10000);
    char* dir = strstr(storage_path,"/S");
    dir++;
    send(sock, dir, strlen(dir), 0);
    printf("Sent storage path to Naming Server: %s\n", storage_path);

    usleep(10000);
    // Step 6: Send the contents of the storage directory to Naming Server

    // Close the connection
    char storage_path_copy[BUFFER_SIZE];
    strncpy(storage_path_copy, storage_path, sizeof(storage_path_copy) - 1);
    storage_path_copy[sizeof(storage_path_copy) - 1] = '\0'; // Ensure null termination

    // Get the parent directory
    char *parent_dir = dirname(storage_path_copy);
    send_directory_contents(sock, storage_path, parent_dir, 0);
    close(sock);
    if (chdir(parent_dir) == -1)
    {
        perror("Failed to change directory to parent of storage path");
        return;
    }

    // printf("Changed directory to parent: %s\n", parent_dir);
}