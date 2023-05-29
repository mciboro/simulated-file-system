#pragma once

#include "../common.h"
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern struct inode_t *inode_table;
extern struct descriptor_t *descriptor_table;
extern struct data_block_t *data_block_table;
extern struct filename_inode_t *filename_table;

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
    unsigned ref_count;
    unsigned data_blocks[FILE_MAX_BLOCKS];
};

struct filename_inode_t {
    struct filename_inode_t *next;
    struct filename_inode_t *prev;
    unsigned node_index;
    char filename[MAX_FILENAME_LEN];
};

struct data_block_t {
    bool allocated;
    unsigned inode_index;
};

int open_inode_table(struct inode_t **head);
int close_inode_table(struct inode_t *head);
int add_inode(struct inode_t **head, fd_type *index, uid_t owner, gid_t owner_group, unsigned ref_count, long mode, off_t st_size,
              struct timespec st_atim, struct timespec st_mtim, struct timespec st_ctim);
int rename_inode(struct inode_t *head, char *oldname, char *newname);
int remove_inode(struct inode_t **head, const char *name);
int chmod_inode(struct inode_t *head, const char *name, unsigned mode);
int create_hard_link(struct inode_t *head, const char *oldname, const char *newname);
int get_file_owner_and_group(struct inode_t *head, const char *name, uid_t *owner, gid_t *group);
int get_file_stat_from_inode(struct inode_t *head, const char *name, struct stat_t *stat);

int add_opened_descriptor(struct descriptor_t **head, unsigned node_index, unsigned mode, unsigned offset);
int close_file_descriptors_table(struct descriptor_t *head);
int remove_descriptor(struct descriptor_t **head, const fd_type desc);

int open_data_block_table(struct data_block_t **head);
int close_data_block_table(struct data_block_t *head);
int occupy_block_table_slot(unsigned inode_index, const char *buf);
int free_block_table_slot(unsigned inode_index);

int open_filename_table(struct filename_inode_t **head);
int close_filename_table(struct filename_inode_t *head);
int add_filename_to_table(struct filename_inode_t **head, const char *name, unsigned node_index);
int get_inode_index_for_filename(struct filename_inode_t *head, const char *name, unsigned *node_index);
int rename_file(struct filename_inode_t *head, const char *oldname, const char *newname);
int check_if_filename_taken(struct filename_inode_t *head, const char *name);