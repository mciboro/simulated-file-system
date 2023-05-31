#pragma once

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>

extern int libfs_errno;
#define IPC_PERMS 0666
#define IPC_REQUESTS_KEY 1234L
#define IPC_RESPONSE_KEY 2345L

#define BUFFER_SIZE 256
#define MAX_FILENAME_LEN 256

#define MAX_MSG_SIZE 4096
#define MAX_MSG_DATA_SIZE MAX_MSG_SIZE - sizeof(struct request_t)

#define DATA_BLOCK_ALLOC_SIZE 4096
#define NUM_OF_DATA_BLOCKS 1000
#define FILE_MAX_BLOCKS 100

typedef unsigned int fd_type;

enum Type { CREATE, CHMODE, RENAME, UNLINK, OPEN, READ, WRITE, SEEK, CLOSE, STAT, LINK, SYMLINK };
enum FileType { F_REGULAR, F_SYMLINK };
enum Status { SUCCESS = 0, FAILURE = -1, FILENAME_TAKEN = -2, FILE_NOT_FOUND = -3 };

struct request_t {
    long seq;
    enum Type type;
    unsigned multipart;
    unsigned data_size;
    unsigned data_offset;
    unsigned part_size;
    char data[];
};

struct response_t {
    long seq;
    enum Status status;
    unsigned multipart;
    unsigned data_size;
    unsigned data_offset;
    unsigned part_size;
    char data[];
};

struct stat_t {
    off_t st_size;           /* Całkowity rozmiar w bajtach */
    struct timespec st_atim; /* Czas ostatniego dostępu */
    struct timespec st_mtim; /* Czas ostatniej modyfikacji */
    struct timespec st_ctim; /* Czas ostatniej zmiany statusu */
};
