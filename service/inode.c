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
        return SUCCESS;
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
        node->type = atoi(token);
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
    return SUCCESS;
}

int add_inode(struct inode_t **head, fd_type *index, unsigned type, uid_t owner, gid_t owner_group, unsigned ref_count,
              long mode, off_t st_size, struct timespec st_atim, struct timespec st_mtim, struct timespec st_ctim) {

    struct inode_t *node = malloc(sizeof(struct inode_t));

    node->type = type;
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
            return SUCCESS;
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
            return SUCCESS;
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
        fprintf(inode_file, "%d,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,", node->index, node->type, node->owner,
                node->owner_group, node->ref_count, node->mode, node->stat.st_size, node->stat.st_atim.tv_sec,
                node->stat.st_mtim.tv_sec, node->stat.st_ctim.tv_sec);
        for (int i = 0; i < FILE_MAX_BLOCKS - 1; i++) {
            fprintf(inode_file, "%d,", node->data_blocks[i]);
        }
        fprintf(inode_file, "%d\n", node->data_blocks[FILE_MAX_BLOCKS - 1]);

        prev = node;
        node = node->next;
        free(prev);
    }

    fclose(inode_file);
    return SUCCESS;
}

int chmod_inode(struct inode_t *head, const char *name, unsigned mode) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == FAILURE) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            node_iter->mode = mode;
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int unlink_inode(struct inode_t *head, const char *name) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            node_iter->ref_count--;
            if (node_iter->ref_count == 0) {
                if (node_iter->prev) {
                    node_iter->prev->next = node_iter->next;
                } else {
                    head = node_iter->next;
                }
                if (node_iter->next) {
                    node_iter->next->prev = node_iter->prev;
                }
                free(node_iter);
            }
            if (remove_filename_from_table(filename_table, name) == -1) {
                syslog(LOG_ERR, "unlink_inode() - Cannot remove inode from filename table");
                return FAILURE;
            }
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int write_inode(struct inode_t *inode, const char *buf, unsigned *offset, unsigned size) {
    unsigned current_block = *offset / DATA_BLOCK_ALLOC_SIZE;
    unsigned new_offset = *offset + size;
    unsigned total_blocks = new_offset / DATA_BLOCK_ALLOC_SIZE + (new_offset % DATA_BLOCK_ALLOC_SIZE != 0);

    if (*offset + size > DATA_BLOCK_ALLOC_SIZE * FILE_MAX_BLOCKS) {
        syslog(LOG_ERR, "write_inode() - File is too big");
        return FAILURE;
    }

    syslog(LOG_INFO, "write_inode() - Current block: %d, Total blocks: %d", current_block, total_blocks);

    unsigned current_offset = 0;
    for (int i = current_block; i < total_blocks; i++) {
        syslog(LOG_INFO, "write_inode() - Current offset: %d", current_offset);
        if (i == current_block && inode->data_blocks[i] != 0) {
            syslog(LOG_INFO, "write_inode() - Updating existing block");
            unsigned size_to_write = size > DATA_BLOCK_ALLOC_SIZE - (*offset % DATA_BLOCK_ALLOC_SIZE)
                                         ? DATA_BLOCK_ALLOC_SIZE - (*offset % DATA_BLOCK_ALLOC_SIZE)
                                         : size;
            if (update_block_table_slot(inode->data_blocks[i] - 1, buf + current_offset,
                                        (*offset % DATA_BLOCK_ALLOC_SIZE), size_to_write) != SUCCESS) {
                syslog(LOG_ERR, "write_inode() - Failed to update block");
                return FAILURE;
            }
            current_offset += size_to_write;
            size -= size_to_write;
        } else {
            syslog(LOG_INFO, "write_inode() - Allocating new block");
            unsigned size_to_write = size > DATA_BLOCK_ALLOC_SIZE ? DATA_BLOCK_ALLOC_SIZE : size;
            unsigned slot_address = 0;
            char *buffer = malloc(DATA_BLOCK_ALLOC_SIZE);
            memset(buffer, 0, DATA_BLOCK_ALLOC_SIZE);
            memcpy(buffer, buf + current_offset, size_to_write);
            if (occupy_block_table_slot(inode->index, buffer, size_to_write, &slot_address) != SUCCESS) {
                syslog(LOG_ERR, "write_inode() - Failed to allocate block");
                free(buffer);
                return FAILURE;
            }
            syslog(LOG_INFO, "write_inode() - New block address: %d", slot_address);
            inode->data_blocks[i] = slot_address;
            current_offset += size_to_write;
            size -= size_to_write;

            free(buffer);
        }
    }
    syslog(LOG_INFO, "write_inode() - New offset: %d", new_offset);
    inode->stat.st_size = new_offset;
    *offset = new_offset;
    return SUCCESS;
}

int write_inode_fd(struct inode_t *head, fd_type fd, const char *buf, unsigned size) {
    struct descriptor_t *fd_iter = descriptor_table;
    while (fd_iter) {
        if (fd_iter->desc == fd) {
            if (fd_iter->mode == READ_ONLY) {
                syslog(LOG_ERR, "write_inode_fd() - File is read only");
                return FAILURE;
            }

            unsigned node_index = fd_iter->node_index;
            struct inode_t *node_iter = head;
            struct inode_t *node = NULL;
            while (node_iter) {
                if (node_iter->index == node_index) {
                    node = node_iter;
                    break;
                }
                node_iter = node_iter->next;
            }
            if (node == NULL) {
                syslog(LOG_ERR, "write_inode_fd() - Node not found");
                return FAILURE;
            }
            if (write_inode(node, buf, &fd_iter->offset, size) != SUCCESS) {
                syslog(LOG_ERR, "write_inode_fd() - Failed to write inode");
                return FAILURE;
            }
            syslog(LOG_INFO, "write_inode_fd() - New offset: %d", fd_iter->offset);
            return SUCCESS;
        }
        fd_iter = fd_iter->next;
    }

    return FAILURE;
}

int read_inode(struct inode_t *inode, char *buf, unsigned *offset, unsigned size) {
    unsigned current_block = *offset / DATA_BLOCK_ALLOC_SIZE;

    if (*offset + size > DATA_BLOCK_ALLOC_SIZE * FILE_MAX_BLOCKS) {
        syslog(LOG_ERR, "read_inode() - File is too big");
        return FAILURE;
    }

    long bytes_to_read = size;
    unsigned current_offset = 0;
    while (bytes_to_read > 0) {
        syslog(LOG_INFO, "read_inode() - Current block: %d, current offset: %d", current_block, current_offset);

        unsigned size_to_read = bytes_to_read > DATA_BLOCK_ALLOC_SIZE - (*offset % DATA_BLOCK_ALLOC_SIZE)
                                    ? DATA_BLOCK_ALLOC_SIZE - (*offset % DATA_BLOCK_ALLOC_SIZE)
                                    : bytes_to_read;
        size_to_read = size_to_read > DATA_BLOCK_ALLOC_SIZE ? DATA_BLOCK_ALLOC_SIZE : size_to_read;
        if (read_block_table_slot(inode->data_blocks[current_block] - 1, buf + current_offset,
                                  (*offset % DATA_BLOCK_ALLOC_SIZE), size_to_read) != SUCCESS) {
            syslog(LOG_ERR, "read_inode() - Failed to read block");
            return FAILURE;
        }
        syslog(LOG_INFO, "read_inode() - Size to read: %d, block address: %d", size_to_read,
               inode->data_blocks[current_block]);
        bytes_to_read -= size_to_read;
        current_offset += size_to_read;
        *offset += size_to_read;
        current_block++;
    }
    return SUCCESS;
}

int read_inode_fd(struct inode_t *head, fd_type fd, char *buf, unsigned size) {
    struct descriptor_t *fd_iter = descriptor_table;
    while (fd_iter) {
        if (fd_iter->desc == fd) {
            unsigned node_index = fd_iter->node_index;
            struct inode_t *node_iter = head;
            struct inode_t *node = NULL;
            while (node_iter) {
                if (node_iter->index == node_index) {
                    node = node_iter;
                    break;
                }
                node_iter = node_iter->next;
            }
            if (node == NULL) {
                syslog(LOG_ERR, "read_inode_fd() - Node not found");
                return FAILURE;
            }
            if (read_inode(node, buf, &fd_iter->offset, size) != SUCCESS) {
                syslog(LOG_ERR, "read_inode_fd() - Failed to read inode");
                return FAILURE;
            }
            syslog(LOG_INFO, "read_inode_fd() - New offset: %d", fd_iter->offset);
            return SUCCESS;
        }
        fd_iter = fd_iter->next;
    }

    return FAILURE;
}

int open_inode(struct inode_t *head, fd_type *index, const char *name, unsigned mode) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }

    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == node_index) {
            // if (node_iter->type == F_SYMLINK) {
            //     char buff[node_iter->stat.st_size];
            //     memset(buff, 0, node_iter->stat.st_size);
            //     unsigned offset = 0;
            //     if (read_inode(node_iter, buff, &offset, node_iter->stat.st_size) != SUCCESS) {
            //         syslog(LOG_ERR, "open_inode() - Failed to read inode");
            //         return FAILURE;
            //     }
            //     if (get_inode_index_for_filename(filename_table, buff, &node_index) == -1) {
            //         syslog(LOG_ERR, "There is no file with name: %s", buff);
            //         return FILE_NOT_FOUND;
            //     }
            //     // Iterate from the beginning
            //     node_iter = head;
            //     continue;
            // }
            *index = add_opened_descriptor(&descriptor_table, node_index, mode, 0);
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int seek_inode_fd(struct inode_t *head, fd_type fd, unsigned offset) {
    struct descriptor_t *fd_iter = descriptor_table;
    while (fd_iter) {
        if (fd_iter->desc == fd) {
            unsigned node_index = fd_iter->node_index;
            struct inode_t *node_iter = head;
            struct inode_t *node = NULL;
            while (node_iter) {
                if (node_iter->index == node_index) {
                    node = node_iter;
                    break;
                }
                node_iter = node_iter->next;
            }
            if (node == NULL) {
                syslog(LOG_ERR, "seek_inode_fd() - Node not found");
                return FAILURE;
            }
            if (offset > node->stat.st_size) {
                syslog(LOG_ERR, "seek_inode_fd() - Offset is too big");
                return FAILURE;
            }
            fd_iter->offset = offset;
            syslog(LOG_INFO, "seek_inode_fd() - New offset: %d", fd_iter->offset);
            return SUCCESS;
        }
        fd_iter = fd_iter->next;
    }

    return FAILURE;
}

int close_inode(const fd_type index) {
    if (remove_descriptor(&descriptor_table, index) != SUCCESS) {
        syslog(LOG_ERR, "close_inode() - Failed to remove descriptor");
        return FAILURE;
    }
    return SUCCESS;
}

int create_hard_link(struct inode_t *head, const char *name, const char *new_name) {
    unsigned node_index = 0;
    // check if file with name exists
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }
    // check if file with new_name exists
    if (check_if_filename_taken(filename_table, new_name)) {
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

int create_soft_link(struct inode_t *head, const char *name, const char *new_name, uid_t owner, gid_t owner_group,
                     long mode) {
    unsigned parent_index = 0, child_index;
    // check if file with name exists
    if (get_inode_index_for_filename(filename_table, name, &parent_index) != SUCCESS) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
    }
    // check if file with new_name exists
    if (check_if_filename_taken(filename_table, new_name)) {
        syslog(LOG_ERR, "There is already a file with name: %s", new_name);
        return FILENAME_TAKEN;
    }

    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME, &curr_time);
    size_t size = strlen(name);
    if (add_inode(&inode_table, &child_index, F_SYMLINK, owner, owner_group, 0, mode, 0, curr_time, curr_time,
                  curr_time) != SUCCESS) {
        syslog(LOG_ERR, "create_soft_link() - Failed to add inode");
        return FAILURE;
    }
    if (add_filename_to_table(&filename_table, new_name, child_index) != SUCCESS) {
        syslog(LOG_ERR, "create_soft_link() - Failed to add filename table entry");
        return FAILURE;
    }

    struct inode_t *parent_node = NULL, *child_node = NULL;
    struct inode_t *node_iter = head;
    while (node_iter) {
        if (node_iter->index == parent_index) {
            parent_node = node_iter;
            break;
        }
        node_iter = node_iter->next;
    }
    node_iter = head;
    while (node_iter) {
        if (node_iter->index == child_index) {
            child_node = node_iter;
            break;
        }
        node_iter = node_iter->next;
    }

    if (!parent_node || !child_node) {
        syslog(LOG_ERR, "Corrupted file nodes!");
        return FAILURE;
    }

    uint8_t buf[parent_node->stat.st_size];
    memset(buf, 0, parent_node->stat.st_size);
    unsigned offset = 0;
    if (read_inode(parent_node, buf, &offset, parent_node->stat.st_size)) {
        syslog(LOG_ERR, "create_soft_link() - Failed to read inode");
        return FAILURE;
    }

    offset = 0;
    if (write_inode(child_node, buf, &offset, parent_node->stat.st_size)) {
        syslog(LOG_ERR, "create_soft_link() - Failed to write inode");
        return FAILURE;
    }
    return SUCCESS;
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
    return SUCCESS;
}

int close_file_descriptors_table(struct descriptor_t *head) {
    struct descriptor_t *node = head, *prev;

    while (node) {
        prev = node;
        node = node->next;
        free(prev);
    }

    return SUCCESS;
}

int remove_descriptor(struct descriptor_t **head, const fd_type desc) {
    struct descriptor_t *desc_iter = *head;
    while (desc_iter) {
        if (desc_iter->desc == desc) {
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
            return SUCCESS;
        }
        desc_iter = desc_iter->next;
    }

    return FAILURE;
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
        return SUCCESS;
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
            return FAILURE;
        }
    }

    fclose(data_block_file);
    return SUCCESS;
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
    return SUCCESS;
}

int occupy_block_table_slot(unsigned inode_index, const char *buf, unsigned buf_size, unsigned *slot_address) {
    for (int i = 0; i < NUM_OF_DATA_BLOCKS; i++) {
        if (!data_block_table[i].allocated) {
            data_block_table[i].allocated = true;
            data_block_table[i].inode_index = inode_index;

            FILE *data_file = NULL;
            uid_t uid = getuid();
            struct passwd *pw = getpwuid(uid);

            if (pw == NULL) {
                syslog(LOG_ERR, "Cannot retrieve info about service's owner");
                exit(EXIT_FAILURE);
            }

            char *libfs_dir = strcat(pw->pw_dir, "/libfs");

            data_file = fopen(strcat(libfs_dir, "/data"), "a+");

            if (!data_file) {
                syslog(LOG_ERR, "occupy_block_table_slot() - Node file descriptor corrupted");
                exit(EXIT_FAILURE);
            }

            uint8_t write_buf[DATA_BLOCK_ALLOC_SIZE] = {0};
            memcpy(write_buf, buf, buf_size);

            fseek(data_file, DATA_BLOCK_ALLOC_SIZE * i, SEEK_SET);
            fwrite(write_buf, DATA_BLOCK_ALLOC_SIZE, 1, data_file);
            fclose(data_file);
            *slot_address = i + 1;
            return SUCCESS;
        }
    }
    return FAILURE;
}

int update_block_table_slot(unsigned block_index, const char *buf, unsigned offset, unsigned size) {
    if (block_index > NUM_OF_DATA_BLOCKS) {
        syslog(LOG_ERR, "Node index corrupted!");
        return FAILURE;
    }

    FILE *data_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");
    char *data_path = strcat(libfs_dir, "/data");

    data_file = fopen(data_path, "r+");
    if (!data_file) {
        syslog(LOG_ERR, "update_block_table_slot() - Node file descriptor corrupted");
        exit(EXIT_FAILURE);
    }

    fseek(data_file, DATA_BLOCK_ALLOC_SIZE * block_index + offset, SEEK_SET);
    fwrite(buf, size, 1, data_file);
    fclose(data_file);
    return SUCCESS;
}

int read_block_table_slot(unsigned block_index, char *buf, unsigned offset, unsigned size) {
    if (block_index > NUM_OF_DATA_BLOCKS) {
        syslog(LOG_ERR, "Node index corrupted!");
        return FAILURE;
    }
    syslog(LOG_INFO, "Reading block %d with size %d", block_index, size);
    FILE *data_block_file = NULL;
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    data_block_file = fopen(strcat(libfs_dir, "/data"), "r+");

    if (!data_block_file) {
        syslog(LOG_ERR, "read_block_table_slot() - Node file descriptor corrupted");
        exit(EXIT_FAILURE);
    }

    fseek(data_block_file, DATA_BLOCK_ALLOC_SIZE * block_index + offset, SEEK_SET);
    fread(buf, size, 1, data_block_file);
    fclose(data_block_file);
    return SUCCESS;
}

int free_block_table_slot(unsigned block_index) {
    if (block_index > NUM_OF_DATA_BLOCKS) {
        syslog(LOG_ERR, "Node index corrupted!");
        return FAILURE;
    }

    data_block_table[block_index].allocated = false;
    data_block_table[block_index].inode_index = -1;
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
        return SUCCESS;
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
    return SUCCESS;
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
    return SUCCESS;
}

int add_filename_to_table(struct filename_inode_t **head, const char *name, unsigned node_index) {
    syslog(LOG_INFO, "Adding filename %s to table", name);
    struct filename_inode_t *node_iter = *head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
            syslog(LOG_INFO, "File already exists!");
            return FILENAME_TAKEN;
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

    return SUCCESS;
}

int get_inode_index_for_filename(struct filename_inode_t *head, const char *name, unsigned *node_index) {
    struct filename_inode_t *node_iter = head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
            *node_index = node_iter->node_index;
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int rename_file(struct filename_inode_t *head, const char *oldname, const char *newname) {
    struct filename_inode_t *node_iter = head;
    if (check_if_filename_taken(head, newname)) {
        return FILENAME_TAKEN;
    }
    while (node_iter) {
        if (strcmp(node_iter->filename, oldname) == 0) {
            memset(node_iter->filename, 0, MAX_FILENAME_LEN);
            strcpy(node_iter->filename, newname);
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}

int remove_filename_from_table(struct filename_inode_t *head, const char *name) {
    struct filename_inode_t *node_iter = head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
            if (node_iter->prev) {
                node_iter->prev->next = node_iter->next;
            }
            if (node_iter->next) {
                node_iter->next->prev = node_iter->prev;
            }
            free(node_iter);
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
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

int remove_inode(struct inode_t **head, const char *name) {
    unsigned node_index = 0;
    if (get_inode_index_for_filename(filename_table, name, &node_index) == -1) {
        syslog(LOG_ERR, "There is no file with name: %s", name);
        return FILE_NOT_FOUND;
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
            return SUCCESS;
        }
        node_iter = node_iter->next;
    }

    return FAILURE;
}