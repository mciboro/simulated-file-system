#pragma once

#include <sys/types.h>
#include <syslog.h>
#include <stdint.h>

extern int errno;
#define IPC_PERMS 0666
#define IPC_REQUESTS_KEY 1234L
#define IPC_RESPONSE_KEY 2345L
#define BUFFER_SIZE 256
#define MAX_MSG_SIZE 8192

typedef unsigned int fd_type;

enum Type { CREATE, CHMODE, RENAME, UNLINK, OPEN, READ, WRITE, SEEK, CLOSE, STAT, LINK, SYMLINK };
enum Status { SUCCESS, FAILURE };

struct request_t {
    pid_t sender;
    enum Type type;
    unsigned multipart;
    unsigned data_size;
    unsigned data_offset;
    unsigned part_size;
    char seq[16]; 
    char data[];
};

struct response_t {
    pid_t receiver;
    enum Status status;
    unsigned multipart;
    unsigned index;
    unsigned data_size;
    char seq[16];
    char rseq[16];
    char data[];
};

struct stat_t {
    off_t st_size;           /* Całkowity rozmiar w bajtach */
    blksize_t st_blksize;    /* Rozmiar bloku dla systemu plików I/O */
    struct timespec st_atim; /* Czas ostatniego dostępu */
    struct timespec st_mtim; /* Czas ostatniej modyfikacji */
    struct timespec st_ctim; /* Czas ostatniej zmiany statusu */
};

void get_uid(char *uid);