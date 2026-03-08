#include "file_operations.h"

// Global variables that need to be shared
extern int PORT;

// File locking global variables
FileLock *lock_list_head = NULL;                             // Head of the linked list for file locks
pthread_mutex_t lock_list_mutex = PTHREAD_MUTEX_INITIALIZER; // Protects access to the lock list

pthread_rwlock_t *get_file_lock(const char *filename)
{
    pthread_rwlock_t *rwlock = NULL;

    // Lock the lock list
    pthread_mutex_lock(&lock_list_mutex);

    // Search for the file lock
    FileLock *current = lock_list_head;
    while (current)
    {
        if (strcmp(current->filename, filename) == 0)
        {
            rwlock = &current->lock;
            break;
        }
        current = current->next;
    }

    // If not found, create a new lock
    if (!rwlock)
    {
        FileLock *new_lock = (FileLock *)malloc(sizeof(FileLock));
        if (!new_lock)
        {
            perror("Failed to allocate memory for file lock");
            pthread_mutex_unlock(&lock_list_mutex);
            return NULL;
        }

        // Initialize the lock
        strcpy(new_lock->filename, filename);
        pthread_rwlock_init(&new_lock->lock, NULL);
        new_lock->next = lock_list_head;
        lock_list_head = new_lock;

        rwlock = &new_lock->lock;
    }

    // Unlock the lock list
    pthread_mutex_unlock(&lock_list_mutex);

    return rwlock;
}

void destroy_all_file_locks()
{
    pthread_mutex_lock(&lock_list_mutex);

    FileLock *current = lock_list_head;
    while (current)
    {
        FileLock *to_free = current;
        pthread_rwlock_destroy(&current->lock);
        current = current->next;
        free(to_free);
    }

    lock_list_head = NULL;
    pthread_mutex_unlock(&lock_list_mutex);
}

void send_file_content(int client_socket, const char *filepath)
{

    struct stat st;
    if (stat(filepath, &st) < 0)
    {
        perror("Failed to get file info");
        char err_mess[50];
        get_error_message(1, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        return;
    }

    // Extract the base name of the file from the file path
    char *base_name = basename((char *)filepath);
    if (!base_name)
    {
        fprintf(stderr, "Failed to extract base name from filepath\n");
        return;
    }

    // Prepare and send header
    file_header header;

    strncpy(header.filename, base_name, 255);
    header.filename[255] = '\0'; // Ensure null-termination

    header.filesize = st.st_size;
    header.type = S_ISDIR(st.st_mode) ? 0 : 1;

    if (send(client_socket, &header, sizeof(header), 0) < 0)
    {
        perror("Failed to send header");
        return;
    }

    // Check if the file is binary
    int is_binary = 0;
    FILE *file_check = fopen(filepath, "rb");
    if (file_check != NULL)
    {
        char ch;
        while (fread(&ch, 1, 1, file_check) == 1)
        {
            if (ch == '\0')
            {
                is_binary = 1;
                break;
            }
        }
        fclose(file_check);
    }

    // Send file content with appropriate encoding
    if (is_binary)
    {
        FILE *file = fopen(filepath, "rb");
        if (file == NULL)
        {
            char err_mess[50];
            write(client_socket, "File not found\n", 15);
            get_error_message(1, err_mess, sizeof(err_mess));
            log_message(2, err_mess);
            return;
        }

        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
        {
            if (send(client_socket, buffer, bytes_read, 0) < 0)
            {
                perror("Failed to send file data");
                fclose(file);
                return;
            }
        }
        fclose(file);
    }
    else
    {
        FILE *file = fopen(filepath, "r");
        if (file == NULL)
        {
            char err_mess[50];
            write(client_socket, "File not found\n", 15);
            get_error_message(1, err_mess, sizeof(err_mess));
            log_message(2, err_mess);
            return;
        }

        char buffer[BUFFER_SIZE];
        while (fgets(buffer, BUFFER_SIZE, file) != NULL)
        {
            write(client_socket, buffer, strlen(buffer));
        }
        fclose(file);
    }
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        char err_mess[50];
        write(client_socket, "File not found\n", 15);
        get_error_message(1, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        return;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, file) != NULL)
    {
        write(client_socket, buffer, strlen(buffer));
    }
    fclose(file);
}

void write_to_file(int client_socket, const char *filename, const char *content)
{
    if (strstr(filename,"/backup_s")){
        write(client_socket, "Can't Write to Backup\n", 23);
        return;
    }
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        char err_mess[50];
        get_error_message(16, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        write(client_socket, "Cannot open file\n", 17);
        return;
    }

    printf("%s\n", content);
    fprintf(file, "%s", content);
    fclose(file);
    write(client_socket, "Write successful\n", 17);
}

void send_file_info(int client_socket, const char *filename)
{
    struct stat file_stat;
    if (stat(filename, &file_stat) < 0)
    {
        char err_mess[50];
        write(client_socket, "File not found\n", 15);
        get_error_message(1, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        return;
    }
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "Size: %ld bytes\nPermissions: %o\n", file_stat.st_size, file_stat.st_mode & 0777);
    write(client_socket, response, strlen(response));
}

// Function to receive a file over network
int receive_file(int sock, const char *dest_path)
{
    file_header header;
    if (recv(sock, &header, sizeof(header), 0) < 0)
    {
        perror("Failed to receive header");
        return -1;
    }

    printf("Received header successfully\n");

    char full_path[PATH_MAX];

    snprintf(full_path, PATH_MAX, "%s/%s", dest_path, header.filename);

    if (header.type == 0)
    { // Directory
        if (mkdir(full_path, 0755) < 0 && errno != EEXIST)
        {
            char err_mess[50];
            get_error_message(11, err_mess, sizeof(err_mess));
            log_message(2, err_mess);
            perror("Failed to create directory");
            return -1;
        }
    }
    else
    { // Regular file
        int fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            char err_mess[50];
            get_error_message(11, err_mess, sizeof(err_mess));
            log_message(2, err_mess);
            perror("Failed to create file");
            return -1;
        }

        size_t remaining = header.filesize;
        char buffer[BUFFER_SIZE];
        while (remaining > 0)
        {
            ssize_t bytes_received = recv(sock, buffer,
                                          MIN(BUFFER_SIZE, remaining), 0);
            if (bytes_received <= 0)
            {
                perror("Failed to receive file data");
                close(fd);
                return -1;
            }

            if (write(fd, buffer, bytes_received) < 0)
            {
                perror("Failed to write file data");
                close(fd);
                return -1;
            }
            remaining -= bytes_received;
        }
        close(fd);
    }
    return 0;
}

void stream_audio(int client_socket, const char *filename)
{
    printf("%s\n", filename);
    int file = open(filename, O_RDONLY);
    if (file < 0)
    {
        write(client_socket, "Audio file not found\n", 21);
        return;
    }
    char buffer[BUFFER_SIZE];
    int bytes;
    while ((bytes = read(file, buffer, BUFFER_SIZE)) > 0)
    {
        write(client_socket, buffer, bytes);
    }
    close(file);
}

void send_directory_contents(int sock, const char *path, char *parent_dir, int send_content)
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    if ((dir = opendir(path)) == NULL)
    {

        perror("Unable to open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Construct the full path of the file/directory
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        char *new_path = full_path + strlen(parent_dir) + 1;

        // Get file/directory information
        if (stat(full_path, &file_stat) == -1)
        {
            perror("stat");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode))
        {
            // It's a directory, send directory name and recurse
            snprintf(buffer, sizeof(buffer), "%s", new_path);
            send(sock, buffer, strlen(buffer), 0);
            usleep(100000);
            send_directory_contents(sock, full_path, parent_dir, send_content); // Recursive call for subdirectories
        }
        else if (S_ISREG(file_stat.st_mode))
        {

            if (!send_content)
            {
                // It's a regular file, send file name
                printf("Sending\n");
                snprintf(buffer, sizeof(buffer), "%s", new_path);
                send(sock, buffer, strlen(buffer), 0);
                usleep(100000);
            }
            else
            {
                printf("Sending informationn about file at %s\n", full_path);
                send_file_content(sock, full_path);
            }
        }
    }

    closedir(dir);
}