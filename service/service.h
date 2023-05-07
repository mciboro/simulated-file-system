#pragma once

#include "common.h"
#include "operations.h"
#include "req_buffer.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 256

struct descriptor_t {
    fd_type desc;
    unsigned mode;
    unsigned offset;
    unsigned ref_count;
    unsigned node_index;
    struct descriptor_t *prev;
    struct descriptor_t *next;
};

struct inode_t {
    unsigned index;
    struct inode_t *prev;
    struct inode_t *next;
    struct stat_t stat;
    char *filename;
};