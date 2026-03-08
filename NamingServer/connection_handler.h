#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "ns.h"

// Function declarations for client and storage server connection handling
void *handle_client(void *client_socket);
void *handle_ss_registration(void *client_socket_ptr);

#endif