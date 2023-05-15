#pragma once

#include "common.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern struct inode_t *inode_table;
extern struct descriptor_t *descriptor_table;
extern FILE *inode_file;

struct descriptor_t {
    fd_type desc;
    unsigned mode;
    unsigned offset;
    unsigned ref_count;
    unsigned node_index;
    struct descriptor_t *next;
    struct descriptor_t *prev;
};

struct inode_t {
    unsigned index;
    struct inode_t *next;
    struct inode_t *prev;
    struct stat_t stat;
    long mode;
    uid_t owner;
    gid_t owner_group;
    char filename[256];
};

int open_inode_table(struct inode_t **head);
int add_inode(struct inode_t **head, char *filename, uid_t owner, gid_t owner_group, long mode, off_t st_size,
              unsigned st_blksize, struct timespec st_atim, struct timespec st_mtim, struct timespec st_ctim);
int close_inode_table(struct inode_t *head);
int remove_inode(struct inode_t **head, const char *name);

int add_opened_descriptor(struct descriptor_t **head, unsigned node_index, unsigned mode, unsigned offset);
int close_file_descriptors_table(struct descriptor_t *head);
int remove_descriptor(struct descriptor_t **head, const fd_type desc);