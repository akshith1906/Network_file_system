#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include "ns.h"

// Function declarations for health monitoring
void init_server_health(int server_index, const char *ip, int port);
bool check_server_health(const char *ip, int port);
void *health_monitor(void *arg);

#endif