#include "backup_sync.h"

void create_backup_path(const char *filename, char *backup_file) {
    // Find the first occurrence of '/' in filename
    const char *slash_pos = strstr(filename, "/");

    if (slash_pos) {
        // Get the position of the slash in the filename
        size_t prefix_len = slash_pos - filename + 1; // Include '/'

        // Copy the prefix (up to and including the '/')
        strncpy(backup_file, filename, prefix_len);
        backup_file[prefix_len] = '\0'; // Null-terminate

        // Append "backup/" and the remaining filename after the '/'
        snprintf(backup_file + prefix_len, 300 - prefix_len, "backup%s", slash_pos);
    } else {
        // No '/' found; just prefix the filename with "/backup/"
        snprintf(backup_file, 300, "/backup/%s", filename);
    }
}

void sync_to_backup(const char *filename, const char *backup_file)
{
    struct stat st;

    // Check if the source file or directory exists
    if (stat(filename, &st) < 0)
    {
        perror("Failed to stat source path");
        return;
    }

    // Handle directory synchronization
    if (S_ISDIR(st.st_mode))
    {
        // Ensure the destination directory exists
        if (mkdir(backup_file, 0755) < 0 && errno != EEXIST)
        {
            perror("Failed to create backup directory");
            return;
        }

        DIR *dir = opendir(filename);
        if (!dir)
        {
            perror("Failed to open source directory");
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Construct full paths for the source and destination
            char src_path[4096];
            char dest_path[4096];
            snprintf(src_path, sizeof(src_path), "%s/%s", filename, entry->d_name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", backup_file, entry->d_name);

            // Recursively sync
            sync_to_backup(src_path, dest_path);
        }

        closedir(dir);
    }
    else if (S_ISREG(st.st_mode))
    {
        // Handle regular file synchronization
        int src_fd, dest_fd;
        char buffer[4096];
        ssize_t bytes_read, bytes_written;

        // Open the source file
        src_fd = open(filename, O_RDONLY);
        if (src_fd < 0)
        {
            perror("Failed to open source file for backup");
            return;
        }

        // Open or create the backup file
        dest_fd = open(backup_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest_fd < 0)
        {
            perror("Failed to open or create backup file");
            close(src_fd);
            return;
        }

        // Copy the content from source to destination
        while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0)
        {
            bytes_written = write(dest_fd, buffer, bytes_read);
            if (bytes_written < 0)
            {
                perror("Failed to write to backup file");
                close(src_fd);
                close(dest_fd);
                return;
            }
        }

        if (bytes_read < 0)
        {
            perror("Failed to read from source file");
        }

        // Close the file descriptors
        close(src_fd);
        close(dest_fd);
    }
    else
    {
        fprintf(stderr, "Unsupported file type for: %s\n", filename);
    }
}

void sync_directory_to_backup(const char *dir, const char *backup_dir)
{
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "cp -r %s %s", dir, backup_dir);
    int result = system(command);

    if (result == -1)
    {
        perror("Failed to sync directory to backup");
    }
    else
    {
        printf("Directory %s successfully copied to backup %s\n", dir, backup_dir);
    }
}

int create_directory(const char *path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0755) == -1)
        {
            char err_mess[50];
            get_error_message(11, err_mess, sizeof(err_mess));
            log_message(2, err_mess);
            perror("mkdir failed");
            return -1;
            // exit(EXIT_FAILURE);
        }
    }
    return 0;
}

void copy_file(const char *source, const char *destination)
{
    FILE *src_file = fopen(source, "rb");
    if (!src_file)
    {
        perror("Error opening source file");
        return;
    }

    FILE *dest_file = fopen(destination, "wb");
    if (!dest_file)
    {
        perror("Error opening destination file");
        fclose(src_file);
        return;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0)
    {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);
}

void duplicate_directory(const char *src_dir, const char *dest_dir)
{
    DIR *dir = opendir(src_dir);
    if (!dir)
    {
        perror("Error opening source directory");
        return;
    }

    struct dirent *entry;
    struct stat entry_stat;

    while ((entry = readdir(dir)) != NULL)
    {
        // Skip special directories "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skip files or directories with names starting with "backup"
        if (strncmp(entry->d_name, "backup", strlen("backup")) == 0)
            continue;

        // Construct full source and destination paths
        char src_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);

        char dest_path[1024];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        // Get entry details
        if (stat(src_path, &entry_stat) == -1)
        {
            perror("Error getting file status");
            continue;
        }

        // If entry is a directory, recursively copy it
        if (S_ISDIR(entry_stat.st_mode))
        {
            create_directory(dest_path);
            duplicate_directory(src_path, dest_path);
        }
        else if (S_ISREG(entry_stat.st_mode)) // If entry is a regular file, copy it
        {
            copy_file(src_path, dest_path);
        }
    }

    closedir(dir);
}

void ss_duplicate(const char *storage_path)
{
    char current_dir[PATH_MAX];
    snprintf(current_dir, PATH_MAX, "%s", storage_path);

    char backup_dir[PATH_MAX];
    snprintf(backup_dir, PATH_MAX, "%s/backup", storage_path);

    // Ensure the backup directory exists or try an alternative name
    if (create_directory(backup_dir) != 0)
    {
        // If the creation of 'backup' fails, try creating 'backup_new'
        snprintf(backup_dir, PATH_MAX, "%s/backup_new", storage_path);
        if (create_directory(backup_dir) != 0)
        {
            // If the creation of 'backup_new' also fails, report an error and exit
            fprintf(stderr, "Error: Unable to create backup directory at %s or %s.\n",
                    storage_path, backup_dir);
            return;
        }
    }

    // Start duplicating the directory
    duplicate_directory(current_dir, backup_dir);

    printf("All files and folders have been duplicated into the 'backup' directory.\n");
}