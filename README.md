# Distributed Network File System (NFS)

[![C Language](https://img.shields.io/badge/Language-C-00599C?logo=c)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Unix/Linux](https://img.shields.io/badge/Platform-Unix/Linux-24273A?logo=linux)](https://en.wikipedia.org/wiki/Unix)
[![Distributed Systems](https://img.shields.io/badge/Category-Distributed_Systems-0077B6)](https://en.wikipedia.org/wiki/Distributed_computing)

C Language Distributed Systems Programming
Table of Contents

- [Project Overview](#project-overview)
- [Features & Technical Highlights](#features--technical-highlights)
  - [Core System Architecture](#core-system-architecture)
  - [Custom Features](#custom-features)
    - [Naming Server - Intelligent Path-to-Server Routing](#naming-server---intelligent-path-to-server-routing)
    - [Storage Server - Distributed File Storage Engine](#storage-server---distributed-file-storage-engine)
    - [Client Interface - Unified Distributed File Access](#client-interface---unified-distributed-file-access)
    - [Advanced Backup System](#advanced-backup-system)
- [Technical Deep Dive](#technical-deep-dive)
  - [Architecture Overview](#architecture-overview)
  - [Core Algorithms & Data Structures](#core-algorithms--data-structures)
- [Setup Instructions](#setup-instructions)
  - [Prerequisites](#prerequisites)
  - [Installation & Compilation](#installation--compilation)
  - [Testing & Verification](#testing--verification)
- [Technical Demonstrations](#technical-demonstrations)
- [Author](#author)

## Project Overview

This is an advanced distributed Network File System (NFS) implementation written entirely in C, designed to demonstrate deep understanding of distributed systems, networking, and systems programming concepts. The system provides a unified interface for storing and accessing files across multiple storage servers through a centralized naming server architecture. This project showcases proficiency in low-level networking, distributed systems design, concurrent programming, and fault tolerance mechanisms.

Key Value Propositions:

- **Distributed Systems Mastery**: Demonstrates advanced understanding of distributed systems architecture, networking protocols, and server coordination
- **Advanced C Programming**: Clean, modular architecture with proper memory management, thread safety, and error handling
- **Educational Implementation**: Comprehensive distributed file system with practical feature set
- **Problem-Solving Skills**: Complex algorithmic implementations for path routing, backup synchronization, and health monitoring
- **Performance Optimization**: Efficient algorithms with O(1) cache lookups and O(path_length) trie operations

## Features & Technical Highlights

### Core System Architecture

- **Distributed Server Architecture**:
  - Central naming server for metadata coordination and routing
  - Multiple storage servers for distributed file storage
  - Client interface for unified access to the distributed filesystem

- **Robust Networking Framework**:
  - TCP/IP communication between all system components
  - Multi-threaded server architecture with thread-safe operations
  - Comprehensive error handling and network timeout management

- **Intelligent File Routing**:
  - Trie-based path-to-storage-server mapping with O(path_length) operations
  - LRU cache for O(1) average path lookups to optimize performance
  - Dynamic routing of client requests to appropriate storage servers

### Custom Features

#### Naming Server - Intelligent Path-to-Server Routing

The naming server serves as the central coordinator, maintaining mappings between file paths and storage servers:

```bash
# System registers storage servers automatically
# Clients query naming server to discover file locations
# Backup synchronization is coordinated automatically
```

Implementation Details: Uses trie data structure for efficient path storage, LRU cache for performance optimization, and health monitoring for server availability tracking.

#### Storage Server - Distributed File Storage Engine

Storage servers provide actual file storage with advanced features:

```bash
# Storage server with file operations and backup
./storage_server 8080 Test/SS1

# Supports multiple operations
WRITE /path/to/file "content" SYNC     # Write with synchronous operations
READ /path/to/file                     # Read file contents
CREATE /path/to/dir 1                  # Create directory
DELETE /path/to/file 0                 # Delete file (0) or directory (1)
STREAM /path/to/audio.mp3              # Stream audio files
```

Technical Notes: Implements read/write locks for concurrent access, supports both synchronous and asynchronous operations, maintains backup copies on other servers in the cluster.

#### Client Interface - Unified Distributed File Access

The client provides a unified command-line interface for the distributed file system:

```bash
# Interactive client session
./client

# Available commands:
READ /file/path              # Read file contents from distributed system
WRITE /file/path "content"   # Write content to file
CREATE /file/path 0          # Create file (0) or directory (1)
DELETE /file/path 0          # Delete file (0) or directory (1)
COPY /src /dest 0            # Copy file (0) or directory (1)
LIST                         # List all files in the system
STREAM /audio.mp3            # Stream audio content
exit                         # Exit client
```

Architecture: Communicates with naming server for path resolution, then directly with storage servers for file operations.

#### Advanced Backup System

Automatic backup synchronization with fault tolerance:

- **Ring-based Backup Topology**: Each server maintains backups on the next two servers in a ring
- **Automatic Recovery**: Failed servers automatically sync missed data when they come back online
- **Backup Consistency**: Ensures data integrity during backup operations
- **Crash Recovery**: Asynchronous writes with temporary file support for recovery

## Technical Deep Dive

### Architecture Overview

The project follows a three-tier distributed architecture with clear separation of concerns:

```
                    ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
                      Network File System (NFS) Architecture
                    └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘

                                    Client Layer
                                    │
                                    ▼
                    ┌─────────────────────────────────┐
                    │         Client Interface        │
                    │  - Command Processing           │
                    │  - User Interaction             │
                    │  - Request Routing              │
                    └─────────────────────────────────┘
                                    │
                                    ▼
                    ┌─────────────────────────────────┐
                    │      Naming Server Layer        │
                    │  - Path-to-Server Mapping       │
                    │  - Trie Data Structure          │
                    │  - LRU Cache                    │
                    │  - Health Monitoring            │
                    │  - Request Coordination         │
                    └─────────────────────────────────┘
                        │       │         │
                        ▼       ▼         ▼
            ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
            │ Storage Server  │ │ Storage Server  │ │ Storage Server  │
            │      1          │ │      2          │ │      N          │
            │  - File Storage │ │  - File Storage │ │  - File Storage │
            │  - Read/Write   │ │  - Read/Write   │ │  - Read/Write   │
            │  - Backup Sync  │ │  - Backup Sync  │ │  - Backup Sync  │
            │  - Streaming    │ │  - Streaming    │ │  - Streaming    │
            └─────────────────┘ └─────────────────┘ └─────────────────┘
                        │       │         │
                        ▼       ▼         ▼
                    ┌─────────────────────────────────┐
                    │        Backup Network          │
                    │  - Ring-based replication      │
                    │  - Automatic failover          │
                    │  - Data consistency            │
                    └─────────────────────────────────┘
```

**Client Layer**: Command-line interface for users to interact with the distributed file system

**Naming Server Layer**: Central coordinator that maintains file path to storage server mappings using trie data structure, with LRU cache for performance optimization

**Storage Server Layer**: Multiple servers that store actual file data, handle file operations, and maintain backup copies of data

**Backup Network**: Ring-based backup synchronization ensuring fault tolerance and data availability

### Refactored Architecture

The system has been redesigned with a modular architecture where each component is separated into focused modules:

- **NamingServer Modules**:
  - **Main Module** (`ns.c`): Core logic and main function
  - **Connection Handler** (`connection_handler.c`): Client and storage server connection management
  - **Health Monitor** (`health_monitor.c`): Server health monitoring and status tracking
  - **Command Processor** (`command_processor.c`): Command parsing and execution
  - **Backup Utilities** (`backup_utils.c`): Backup synchronization utilities
  - **Core Services** (`trie.c`, `lru.c`, `log.c`, `ns_commands.c`): Fundamental services

- **StorageServer Modules**:
  - **Main Module** (`ss.c`): Core logic and main function
  - **File Operations** (`file_operations.c`): File read/write/stream and locking mechanisms
  - **Network Communications** (`network_comm.c`): Client handling and network communication
  - **Backup Synchronization** (`backup_sync.c`): Backup creation and synchronization
  - **Async Operations** (`async.c`): Asynchronous operations with crash recovery
  - **Logging** (`log.c`): Logging services

This modular design improves maintainability, testability, and enables easier enhancements to individual components.


```
├── Clients/                    # Client interface implementation
│   └── client.c                # Main client logic with command processing
├── NamingServer/               # Central coordination and metadata management
│   ├── ns.c                    # Main naming server logic with main function
│   ├── ns.h                    # Main header with data structures
│   ├── connection_handler.h/c  # Client and server connection handling functions
│   ├── health_monitor.h/c      # Health monitoring and server status functions
│   ├── command_processor.h/c   # Command processing and execution functions
│   ├── backup_utils.h/c        # Backup and synchronization utility functions
│   ├── ns_commands.c           # Network communication functions
│   ├── trie.c                  # Trie structure for path-to-server mapping
│   ├── lru.c                   # LRU cache implementation
│   └── log.c                   # Logging functionality
├── StorageServer/              # Distributed file storage engine
│   ├── ss.c                    # Main storage server logic with main function
│   ├── ss.h                    # Storage server main header with constants
│   ├── file_operations.h/c     # File read/write/stream operations and locking
│   ├── network_comm.h/c        # Network communication and client handling
│   ├── backup_sync.h/c         # Backup and synchronization functions
│   ├── async.h/c               # Asynchronous operations with recovery
│   └── log.c                   # Logging functionality
├── Utils/                      # Common utilities and constants
│   ├── constants.h             # System constants and configuration
│   ├── headers.h               # Common includes
│   ├── errors.c                # Error handling system
│   └── errors.h                # Error message definitions
├── Test/                       # Test directories and data
├── makefile                    # Build configuration
└── make.sh                     # Automated startup script
```

### Core Algorithms & Data Structures

- **Trie Data Structure for Path Storage**:
  - O(path_length) insertion and retrieval operations
  - ASCII character-based indexing (256 children per node)
  - Efficient memory usage with shared path prefixes
  - Thread-safe operations with mutex protection

- **LRU Cache for Path Resolution**:
  - O(1) average case path lookups using hash table
  - Doubly-linked list for LRU ordering
  - Automatic capacity management with eviction policy
  - Thread-safe operations with proper synchronization

- **File Locking System**:
  - Read/write locks for concurrent file access
  - Dynamic lock creation and management with linked list
  - Thread-safe lock allocation and deallocation
  - Prevention of race conditions during concurrent operations

- **Backup Synchronization Algorithm**:
  - Ring-based topology for distributed backup
  - Automatic sync when servers come back online
  - Consistent backup creation using temporary files
  - Asynchronous backup operations to minimize performance impact

## Setup Instructions

### Prerequisites

- Linux/Unix operating system
- GCC (GNU Compiler Collection) installed
- Standard C development libraries
- `make` utility
- `gnome-terminal` (for automated startup script with multiple windows)
- Development libraries for pthread, socket programming, and system calls

### Installation & Compilation

```bash
# Clone the repository
git clone <repository-url>
cd Network-File-System

# Compile all components using makefile
make clean && make

# Or compile manually
gcc -o naming_server NamingServer/ns.c NamingServer/log.c NamingServer/trie.c NamingServer/ns_commands.c NamingServer/lru.c Utils/errors.c -lpthread -fsanitize=address -g
gcc -o storage_server StorageServer/server_commands.c StorageServer/async.c Utils/errors.c StorageServer/log.c -lpthread -fsanitize=address -g
gcc -o client Clients/client.c Utils/errors.c -lpthread -fsanitize=address -g
```

### Running the System

#### Automated Startup with Multiple Terminals

```bash
# This will start naming server, 2 storage servers, and a client in separate terminals
./make.sh
```

#### Manual Startup (Recommended for Development)

1. Start the naming server:
```bash
./naming_server
```

2. Start storage servers in separate terminals:
```bash
./storage_server 8080 Test/SS1
./storage_server 7070 Test/SS2
./storage_server 6060 Test/SS3
```

3. Start the client:
```bash
./client
```

## Testing & Verification

After compilation and startup, test basic functionality:

```bash
# In client terminal, try the following commands:
CREATE /my_file 0          # Create a file
WRITE /my_file "Hello, distributed world!"  # Write content
READ /my_file              # Read the content
LIST                       # List all files in the system
CREATE /my_dir 1           # Create a directory
DELETE /my_file 0          # Delete the file
exit                       # Exit the client
```

Expected Output:
- Naming server should show routing logs
- Storage servers should show file operation logs
- Client should display results of operations
- All components should maintain consistent state

## Technical Demonstrations

### Distributed Systems Concepts

- **Centralized Coordination**: Naming server manages metadata and routes requests
- **Network Communication**: TCP/IP socket programming with error handling
- **Distributed File Storage**: Data stored across multiple servers with coordination
- **Backup and Fault Tolerance**: Automatic backup synchronization and recovery

### Advanced Programming Techniques

- **Thread Safety**: Mutexes and locks for concurrent access protection
- **Memory Management**: Proper allocation and deallocation with error checking
- **Performance Optimization**: LRU caching and efficient trie-based path storage
- **Error Handling**: Comprehensive error detection and reporting system

### Network Programming Concepts

- **TCP/IP Socket Programming**: Server-client communication implementation
- **Multi-threaded Servers**: Concurrent request handling with pthreads
- **Network Protocols**: Custom protocol for inter-server communication
- **Connection Management**: Proper connection establishment and cleanup






## Refactoring Achieved

The codebase has undergone significant refactoring to improve maintainability and readability:

### Completed Refactoring

#### 1. NamingServer Module
- **Original**: `ns.c` (824 lines) contained too many responsibilities
- **Completed**: Successfully split into separate focused modules:
  - **Main Logic** (`ns.c`): Core logic and main function
  - **Connection Handler** (`connection_handler.c/h`): Client and storage server connection management
  - **Health Monitor** (`health_monitor.c/h`): Server health monitoring and status tracking
  - **Command Processor** (`command_processor.c/h`): Command parsing and execution
  - **Backup Utilities** (`backup_utils.c/h`): Backup synchronization utilities

#### 2. StorageServer Module
- **Original**: `server_commands.c` (1248 lines) was extremely large and complex
- **Completed**: Successfully decomposed into multiple focused files:
  - **Main Logic** (`ss.c`): Core logic and main function
  - **File Operations** (`file_operations.c/h`): File I/O operations and locking mechanisms
  - **Network Communications** (`network_comm.c/h`): Network communication and client handling
  - **Backup Synchronization** (`backup_sync.c/h`): Backup creation and synchronization
  - **Async Operations** (`async.c/h`): Asynchronous operations with crash recovery

#### 3. Improved Architecture
- **Result**: Clean separation of concerns with focused modules
- **Benefits**: Better maintainability, testability, and extensibility
- **Performance**: Improved modularity without sacrificing efficiency

## Author

M Akshith
Email: mavurapu2004@gmail.com
