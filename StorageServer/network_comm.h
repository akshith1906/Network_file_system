#ifndef NETWORK_COMM_H
#define NETWORK_COMM_H

#include "ss.h"

// Network communication functions
void *handle_client(void *client_socket);
void register_with_naming_server(int PORT, const char *storage_path);

#endif