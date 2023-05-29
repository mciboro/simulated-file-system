#include "inode.h"

struct inode_t *inode_table = NULL;
struct descriptor_t *descriptor_table = NULL;
struct data_block_t *data_block_table = NULL;
struct filename_inode_t *filename_table = NULL;

int open_inode_table(struct inode_t **head) {
    FILE *inode_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    inode_file = fopen(strcat(libfs_dir, "/inodes"), "r");

    if (!inode_file) {
        syslog(LOG_ERR, "open_inode_table() - No file node");
        return 0;
    }

    char line[4096] = {0}, *token;
    while (fgets(line, sizeof(line), inode_file)) {
        struct inode_t *node = malloc(sizeof(struct inode_t));
        unsigned copy_off = 0;
        memset(node, 0, sizeof(struct inode_t));

        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->index = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->owner = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->owner_group = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->ref_count = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->mode = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->stat.st_size = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->stat.st_atim.tv_sec = atoi(token);
        node->stat.st_atim.tv_nsec = atoi(token) * 1000000;
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->stat.st_mtim.tv_sec = atoi(token);
        node->stat.st_mtim.tv_nsec = atoi(token) * 1000000;
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->stat.st_ctim.tv_sec = atoi(token);
        node->stat.st_ctim.tv_nsec = atoi(token) * 1000000;
        for (int i = 0; i < FILE_MAX_BLOCKS - 1; i++) {
            token = strtok(line + copy_off, ",");
            copy_off += strlen(token) + 1;
            node->data_blocks[i] = atoi(token);
        }
        token = strtok(line + copy_off, "\n");
        copy_off += strlen(token) + 1;
        node->data_blocks[FILE_MAX_BLOCKS - 1] = atoi(token);

        node->next = NULL;
        node->prev = NULL;

        if (*head == NULL) {
            *head = node;
        } else {
            struct inode_t *curr = *head;
            while (curr->next != NULL) {
                curr = curr->next;
            }
            curr->next = node;
            node->prev = curr;
        }

        memset(line, 0, 4096);
    }

    fclose(inode_file);
    return 0;
}

int add_inode(struct inode_t **head, fd_type *index, uid_t owner, gid_t owner_group, unsigned ref_count, long mode, off_t st_size,
              struct timespec st_atim, struct timespec st_mtim, struct timespec st_ctim) {

    struct inode_t *node = malloc(sizeof(struct inode_t));

    node->owner = owner;
    node->owner_group = owner_group;
    node->ref_count = ref_count;
    node->mode = mode;
    node->stat.st_size = st_size;
    node->stat.st_atim = st_atim;
    node->stat.st_mtim = st_mtim;
    node->stat.st_ctim = st_ctim;
    memset(node->data_blocks, 0, sizeof(unsigned) * FILE_MAX_BLOCKS);
    node->next = NULL;
    node->prev = NULL;

    if (*head == NULL) {
        node->index = 0;
        *head = node;
    } else {
        struct inode_t *curr = *head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        node->index = curr->index + 1;
        curr->next = node;
        node->prev = curr;
    }

    *index = node->index;
    return SUCCESS;
}

int get_file_owner_and_group(struct inode_t *head, const char *name, uid_t *owner, gid_t *group) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            *owner = node_iter->owner;
            *group = node_iter->owner_group;
            return 0;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int get_file_stat_from_inode(struct inode_t *head, const char *name, struct stat_t *stat) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            memcpy(stat, &node_iter->stat, sizeof(struct stat_t));
            return 0;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int close_inode_table(struct inode_t *head) {

    FILE *inode_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    inode_file = fopen(strcat(libfs_dir, "/inodes"), "w");

    if (!inode_file) {
        syslog(LOG_ERR, "close_inode_table() - Node file descriptor corrupted");
        exit(EXIT_FAILURE);
    }

    struct inode_t *node = head, *prev;

    fseek(inode_file, 0, SEEK_SET);

    while (node) {
        fprintf(inode_file, "%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,", node->index, node->owner, node->owner_group, node->ref_count, node->mode,
                node->stat.st_size, node->stat.st_atim.tv_sec, node->stat.st_mtim.tv_sec, node->stat.st_ctim.tv_sec);
        for (int i = 0; i < FILE_MAX_BLOCKS - 1; i++) {
            fprintf(inode_file, "%d,", node->data_blocks[i]);
        }
        fprintf(inode_file, "%d\n", node->data_blocks[FILE_MAX_BLOCKS - 1]);

        prev = node;
        node = node->next;
        free(prev);
    }

    fclose(inode_file);
    return 0;
}

int chmod_inode(struct inode_t *head, const char *name, unsigned mode) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return -1;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            node_iter->mode = mode;
            return 0;
        }
        node_iter = node_iter->next;
    }

    return -1;
}

int create_hard_link(struct inode_t *head, const char *name, const char *new_name) {
    unsigned node_index = 0;
    // check if file with name exists
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }
    // check if file with new_name exists
    if (get_inode_index_for_filename(filename_table, new_name, &node_index) != -1) {
        syslog(LOG_ERR, "There is already a file with name: %s", new_name);
        return FILENAME_TAKEN;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            add_filename_to_table(&filename_table, new_name, node_iter->index);
            node_iter->ref_count++;
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int remove_inode(struct inode_t **head, const char *name) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return -1;
    }

    struct inode_t *node_iter = *head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            if (node_iter->prev) {
                node_iter->prev->next = node_iter->next;
            } else {
                if (node_iter->next) {
                    node_iter->next->prev = NULL;
                }
                *head = node_iter->next;
            }

            if (node_iter->next) {
                node_iter->next->prev = node_iter->prev;
            } else {
                if (node_iter->prev) {
                    node_iter->prev->next = NULL;
                }
            }
            free(node_iter);
            return 0;
        }
        node_iter = node_iter->next;
    }

    return -1;
}

int add_opened_descriptor(struct descriptor_t **head, unsigned node_index, unsigned mode, unsigned offset) {
    struct descriptor_t *node = malloc(sizeof(struct descriptor_t));
    node->mode = mode;
    node->node_index = node_index;
    node->offset = offset;
    node->next = NULL;
    node->prev = NULL;

    if (*head == NULL) {
        node->desc = 0;
        *head = node;
    } else {
        struct descriptor_t *curr = *head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        node->desc = curr->desc + 1;
        curr->next = node;
        node->prev = curr;
    }

    return node->desc;
    return 0;
}

int close_file_descriptors_table(struct descriptor_t *head) {
    struct descriptor_t *node = head, *prev;

    while (node) {
        prev = node;
        node = node->next;
        free(prev);
    }

    return 0;
}

int remove_descriptor(struct descriptor_t **head, const fd_type desc) {
    struct descriptor_t *desc_iter = *head;
    while (desc_iter) {
        if (desc_iter->desc = desc) {
            if (desc_iter->prev) {
                desc_iter->prev->next = desc_iter->next;
            } else {
                if (desc_iter->next) {
                    desc_iter->next->prev = NULL;
                }
                *head = desc_iter->next;
            }

            if (desc_iter->next) {
                desc_iter->next->prev = desc_iter->prev;
            } else {
                if (desc_iter->prev) {
                    desc_iter->prev->next = NULL;
                }
            }
            free(desc_iter);
            return 0;
        }
        desc_iter = desc_iter->next;
    }

    return -1;
}

int open_data_block_table(struct data_block_t **head) {

    data_block_table = malloc(NUM_OF_DATA_BLOCKS * sizeof(struct data_block_t));
    memset(data_block_table, 0, NUM_OF_DATA_BLOCKS * sizeof(struct data_block_t));

    FILE *data_block_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    data_block_file = fopen(strcat(libfs_dir, "/data-blocks"), "r");

    if (!data_block_file) {
        syslog(LOG_ERR, "open_data_block_table() - No file node");
        return 0;
    }

    char line[4096] = {0}, *token;
    unsigned index = 0;
    while (fgets(line, sizeof(line), data_block_file)) {
        unsigned copy_off = 0;

        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        data_block_table[index].allocated = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        data_block_table[index].inode_index = atoi(token);
        token = strtok(line + copy_off, ",");

        memset(line, 0, 4096);
        index++;

        if (index >= NUM_OF_DATA_BLOCKS) {
            fclose(data_block_file);
            syslog(LOG_ERR, "Too much data blocks taken. Run out of memory!");
            return -1;
        }
    }

    fclose(data_block_file);
    return 0;
}

int close_data_block_table(struct data_block_t *head) {

    FILE *data_block_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    data_block_file = fopen(strcat(libfs_dir, "/data-blocks"), "w");

    if (!data_block_file) {
        syslog(LOG_ERR, "close_data_block_table() - Node file descriptor corrupted");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_OF_DATA_BLOCKS; i++) {
        fprintf(data_block_file, "%d,%d\n", data_block_table[i].allocated, data_block_table[i].inode_index);
    }

    fclose(data_block_file);
    free(data_block_table);
    return 0;
}

int occupy_block_table_slot(unsigned inode_index, const char *buf) {
    for (int i = 0; i < NUM_OF_DATA_BLOCKS; i++) {
        if (!data_block_table[i].allocated) {
            data_block_table[i].allocated = true;
            data_block_table[i].inode_index = inode_index;

            FILE *data_block_file = NULL;
            uid_t uid = getuid();
            struct passwd *pw = getpwuid(uid);

            if (pw == NULL) {
                syslog(LOG_ERR, "Cannot retrieve info about service's owner");
                exit(EXIT_FAILURE);
            }

            char *libfs_dir = strcat(pw->pw_dir, "/libfs");

            data_block_file = fopen(strcat(libfs_dir, "/data"), "a+");

            if (!data_block_file) {
                syslog(LOG_ERR, "occupy_block_table_slot() - Node file descriptor corrupted");
                exit(EXIT_FAILURE);
            }

            fseek(data_block_file, DATA_BLOCK_ALLOC_SIZE * i, SEEK_SET);
            fwrite(buf, DATA_BLOCK_ALLOC_SIZE, 1, data_block_file);
            fclose(data_block_file);
            return 0;
        }

        return -1;
    }
}

int free_block_table_slot(unsigned inode_index) {
    if (inode_index > NUM_OF_DATA_BLOCKS) {
        syslog(LOG_ERR, "Node index corrupted!");
        return -1;
    }

    data_block_table[inode_index].allocated = false;
    data_block_table[inode_index].inode_index = -1;
}

int open_filename_table(struct filename_inode_t **head) {
    FILE *filename_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    filename_file = fopen(strcat(libfs_dir, "/filenames"), "r");

    if (!filename_file) {
        syslog(LOG_ERR, "open_filename_table() - No file node");
        return 0;
    }

    char line[4096] = {0}, *token;
    while (fgets(line, sizeof(line), filename_file)) {
        struct filename_inode_t *node = malloc(sizeof(struct filename_inode_t));
        unsigned copy_off = 0;
        memset(node, 0, sizeof(struct filename_inode_t));

        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->node_index = atoi(token);
        token = strtok(line + copy_off, "\n");
        copy_off += strlen(token) + 1;
        strcpy(node->filename, token);

        node->next = NULL;
        node->prev = NULL;

        if (*head == NULL) {
            *head = node;
        } else {
            struct filename_inode_t *curr = *head;
            while (curr->next != NULL) {
                curr = curr->next;
            }
            curr->next = node;
            node->prev = curr;
        }

        memset(line, 0, 4096);
    }

    fclose(filename_file);
    return 0;
}

int close_filename_table(struct filename_inode_t *head) {

    FILE *filename_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    filename_file = fopen(strcat(libfs_dir, "/filenames"), "w");

    if (!filename_file) {
        syslog(LOG_ERR, "close_filename_table() - Node file descriptor corrupted");
        exit(EXIT_FAILURE);
    }

    struct filename_inode_t *node = head, *prev;

    fseek(filename_file, 0, SEEK_SET);

    while (node) {
        fprintf(filename_file, "%d,%s\n", node->node_index, node->filename);

        prev = node;
        node = node->next;
        free(prev);
    }

    fclose(filename_file);
    return 0;
}

int add_filename_to_table(struct filename_inode_t **head, const char *name, unsigned node_index) {
    struct filename_inode_t *node_iter = *head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
            syslog(LOG_INFO, "File already exists!");
            return -1;
        }
        node_iter = node_iter->next;
    }

    struct filename_inode_t *node = malloc(sizeof(struct filename_inode_t));

    node->node_index = node_index;
    strcpy(node->filename, name);
    node->next = NULL;
    node->prev = NULL;

    if (*head == NULL) {
        *head = node;
    } else {
        struct filename_inode_t *curr = *head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = node;
        node->prev = curr;
    }

    return 0;
}

int get_inode_index_for_filename(struct filename_inode_t *head, const char *name, unsigned *node_index) {
    struct filename_inode_t *node_iter = head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
            *node_index = node_iter->node_index;
            return 0;
        }
        node_iter = node_iter->next;
    }

    return -1;
}

int rename_file(struct filename_inode_t *head, const char *oldname, const char *newname) {
    struct filename_inode_t *node_iter = head;
    if (check_if_filename_taken(head, newname)) {
        return -2;
    } 
    while (node_iter) {
        if (strcmp(node_iter->filename, oldname) == 0) {
            memset(node_iter->filename, 0, MAX_FILENAME_LEN);
            strcpy(node_iter->filename, newname);
            return 0;
        }
        node_iter = node_iter->next;
    }

    return -1;    
}

int check_if_filename_taken(struct filename_inode_t *head, const char *name) {
    struct filename_inode_t *node_iter = head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
            return true;
        }
        node_iter = node_iter->next;
    }

    return false;
}
