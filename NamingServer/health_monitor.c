#include "health_monitor.h"

extern serdest src_dest[MAX_STORAGE_SERVERS];
extern pthread_mutex_t mutex;
extern StorageServer storage_servers[MAX_STORAGE_SERVERS];
extern int server_count;
extern ServerHealth server_health[MAX_STORAGE_SERVERS];
extern int async_writing[MAX_STORAGE_SERVERS];

// Initialize health monitoring for a server
void init_server_health(int server_index, const char *ip, int port)
{
    pthread_mutex_lock(&health_mutex);
    strncpy(server_health[server_index].ip, ip, INET_ADDRSTRLEN);
    server_health[server_index].port = port;
    server_health[server_index].is_active = true;
    server_health[server_index].last_seen = time(NULL);
    server_health[server_index].failed_checks = 0;
    pthread_mutex_init(&server_health[server_index].health_mutex, NULL);
    pthread_mutex_unlock(&health_mutex);
}

// Check if a server is responsive
bool check_server_health(const char *ip, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = 2; // 2 second timeout
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof tv);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sock);
        return false;
    }

    close(sock);
    return true;
}

// Monitor thread function
void *health_monitor(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&health_mutex);
        for (int i = 0; i < server_count; i++)
        {
            pthread_mutex_lock(&server_health[i].health_mutex);

            bool current_status = check_server_health(server_health[i].ip, server_health[i].port);

            if (!current_status)
            {
                server_health[i].failed_checks++;

                if (server_health[i].failed_checks >= MAX_RETRIES && server_health[i].is_active)
                {
                    // Server just went down
                    server_health[i].is_active = false;
                    log_message(LOG_WARNING, "Storage server %s:%d is DOWN after %d failed health checks",
                                server_health[i].ip, server_health[i].port, MAX_RETRIES);

                    if (async_writing[i] == 1)
                    {
                        async_writing[i] = 2;
                    }
                }
            }
            else
            {
                if (!server_health[i].is_active)
                {
                    // Server just came back up
                    log_message(LOG_INFO, "Storage server %s:%d is back ONLINE",
                                server_health[i].ip, server_health[i].port);
                }
                server_health[i].is_active = true;
                server_health[i].is_active = true;
                server_health[i].failed_checks = 0;
                server_health[i].last_seen = time(NULL);
            }

            pthread_mutex_unlock(&server_health[i].health_mutex);
        }
        pthread_mutex_unlock(&health_mutex);

        sleep(HEALTH_CHECK_INTERVAL);
    }
    return NULL;
}