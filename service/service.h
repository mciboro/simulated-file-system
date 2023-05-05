#pragma once

#include <semaphore.h>
#include "common.h"
#include "req_buffer.h"

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