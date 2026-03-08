#include "command_processor.h"

extern StorageServer storage_servers[MAX_STORAGE_SERVERS];
extern int server_count;
extern pthread_mutex_t mutex;
extern TrieNode *file_trie;
extern LRUCache *cache;

void handle_ns_commands(char *command_buffer)
{
    // printf("1");
    int ARGUMENT_BUFFER_SIZE = 256;
    char command[ARGUMENT_BUFFER_SIZE];
    char path1[ARGUMENT_BUFFER_SIZE];
    char path2[ARGUMENT_BUFFER_SIZE];
    int type = -1;

    // Initialize the variables to empty strings
    memset(command, 0, sizeof(command));
    memset(path1, 0, sizeof(path1));
    memset(path2, 0, sizeof(path2));

    // Parse the command and arguments
    int args = sscanf(command_buffer, "%s %s %s %d", command, path1, path2, &type);
    // Print out all the commands and arguments space separated
    printf("Command: %s\n", command);
    // printf("%d\n", args);

    if (args >= 2)
    {
        printf("Argument 1: %s\n", path1);
    }
    if (args >= 3)
    {
        printf("Argument 2: %s\n", path2);
    }
    if (args == 4)
    {
        printf("Argument 3: %d\n", type);
    }

    // Handle commands
    // printf("here\n");
    // if(file_trie == NULL)
    // {
    //     printf("null");
    // }
    if ((strcmp(command, "CREATE") == 0) && (args == 4))
    {
        // printf("here");
        // printf("%d", type);

        StorageServer *cr = find_storage_server(file_trie, path1);
        if (cr == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return;
        }
        StorageServer cre = *cr;
        send_create(cre.ip, cre.port, path1, type);
        // create(path1, type);
        printf("CREATE command executed successfully.\n");
    }
    else if (strcmp(command, "DELETE") == 0 && args == 4)
    {
        // list_directory(path1);
        StorageServer *cre_ptr = find_storage_server(file_trie, path1);
        if (cre_ptr == NULL) {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return;
        }
        StorageServer cre = *cre_ptr;
        if(&cre == NULL)
        {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return;
        }
        printf("path2=%s\n", path2);
        send_delete(cre.ip, cre.port, path2, type);
        delete_path(file_trie,path2,cre.ip, cre.port);
        // delete (path1, type);
        printf("DELETE command executed successfully.\n");
    }
    else if (strcmp(command, "COPY") == 0 && args >= 3)
    {
        // copy_file(path1, path2);
        StorageServer *src_ptr = find_storage_server(file_trie, path1);
        StorageServer *dest_ptr = find_storage_server(file_trie, path2);

        if (src_ptr == NULL || dest_ptr == NULL) {
            char err_mess[50];
            get_error_message(4, err_mess, sizeof(err_mess));
            log_message(3, err_mess);
            return;
        }

        StorageServer src = *src_ptr;
        StorageServer dest = *dest_ptr;

        printf("Source Server Port: %d\n", src.port);
        printf("Destination Server Port: %d\n", dest.port);

        if (args == 3)
        {
            copy_file_network(path1, path2, src.ip, dest.ip, src.port, dest.port);
            printf("COPY_FILE command executed successfully.\n");
            char combined_path[PATH_MAX];
            snprintf(combined_path, PATH_MAX, "%s/%s", path2, path1);
            insert_path(file_trie, combined_path, dest.ip, dest.port);
        }
        else if (args == 4 && type == 1)
        {
            // copy_directory(path1, path2);

            copy_directory_network(path1, path2, src.ip, dest.ip, src.port, dest.port, 1);
            printf("COPY_DIR command executed successfully.\n");

        }
        else
        {
            printf("Usage:\n");
            printf("COPY <source_path> <destination_path> <0/1>\n");
        }
    }
    else if (strcmp(command, "LIST") == 0)
    {
        // Send "hi" to the client
        print_all_paths(file_trie);
    }
    else if (strcmp(command, "QUIT") == 0)
    {
        printf("QUIT command received. Exiting command handler.\n");
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }
}