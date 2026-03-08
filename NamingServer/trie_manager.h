#ifndef TRIE_MANAGER_H
#define TRIE_MANAGER_H

#include "ns.h"

// Function declarations for trie management
TrieNode *create_node();
void insert_path(TrieNode *root, const char *path, char *server_ip, int server_port);
void print_paths_recursive(TrieNode *node, char *buffer, int depth);
void print_all_paths(TrieNode *root);
void free_trie(TrieNode *node);
bool has_children(TrieNode *node);
bool delete_specific_path(TrieNode *node, const char *path, int depth, const char *server_ip, int server_port);
void delete_path(TrieNode *root, const char *path, const char *server_ip, int server_port);
StorageServer *find_storage_server(TrieNode *root, const char *path);
print_all_paths_to_client(TrieNode* root,int Client_socket);
void reroute_prefix(TrieNode *root, char *src_path, char *dest_path, char* dest_ip, int dest_port);
void copy_subtree(TrieNode *src, TrieNode *dest, char* dest_ip, int dest_port);

#endif