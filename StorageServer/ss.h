#include "../Utils/constants.h"
#include "../Utils/headers.h"
#include "../Utils/typedefs.h"
#include "../Utils/errors.h"

// #define PORT 7070
// #define STORAGE_PORT 7070

// Log definitions
#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_FILE "storage_server.log"

// Include necessary constants from Utils (NM_IP defined in constants.h)
// #define NM_IP "192.168.195.214"  // IP address of the Naming Server - REMOVED: Defined in constants.h
// #define NM_PORT 5000       // Port of the Naming Server - REMOVED: Defined in constants.h
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Include logging functionality
#include "log.h"

extern int PORT;