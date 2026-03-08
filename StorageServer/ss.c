#include "ss.h"
#include "file_operations.h"
#include "network_comm.h"
#include "backup_sync.h"
#include "async.h"

int PORT = 0; // Global variable for port

int main(int argc, char *argv[])
{
    if (argc != 3) // Expecting two arguments: PORT and storage path
    {
        char err_mess[50];
        get_error_message(4, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        fprintf(stderr, "Usage: %s <PORT> <STORAGE_PATH>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    PORT = atoi(argv[1]);
    const char *storage_path = argv[2];

    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Initialize storage server duplication
    ss_duplicate(storage_path);
    
    // Register with Naming Server with both PORT and storage path
    register_with_naming_server(PORT, storage_path);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        char err_mess[50];
        get_error_message(9, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        char err_mess[50];
        get_error_message(9, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        char err_mess[50];
        get_error_message(9, err_mess, sizeof(err_mess));
        log_message(2, err_mess);
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Storage Server listening on port %d\n", PORT);

    // Server loop to accept and handle incoming connections
    while (1)
    {
        // Accept a new client connection
        char temp_buffer[BUFFER_SIZE];
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
        {
            char err_mess[50];
            get_error_message(9, err_mess, sizeof(err_mess));
            log_message(2, err_mess);
            perror("Accept failed");
            continue;
        }

        int bytes_received = recv(client_socket, temp_buffer, sizeof(temp_buffer) - 1, MSG_PEEK);
        if (bytes_received > 0)           // Handle client requests
            handle_client(&client_socket); // fail

        // Close the client socket after handling
        close(client_socket);
    }

    // Close the server socket before exiting
    close(server_fd);
    return 0;
}