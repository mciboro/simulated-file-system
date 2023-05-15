#include "inode.h"

struct inode_t *inode_table = NULL;
struct descriptor_t *descriptor_table = NULL;

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
        syslog(LOG_ERR, "service_create() - No file node");
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
        node->mode = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->stat.st_size = atoi(token);
        token = strtok(line + copy_off, ",");
        copy_off += strlen(token) + 1;
        node->stat.st_blksize = atoi(token);
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
        token = strtok(line + copy_off, "\n");
        copy_off += strlen(token) + 1;
        strcpy(node->filename, token);

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

int add_inode(struct inode_t **head, char *filename, uid_t owner, gid_t owner_group, long mode, off_t st_size,
              unsigned st_blksize, struct timespec st_atim, struct timespec st_mtim, struct timespec st_ctim) {
    struct inode_t *node_iter = *head;
    while (node_iter) {
        if (strcmp(node_iter->filename, filename) == 0) {
            syslog(LOG_INFO, "File already exists!");
            return node_iter->index;
        }
        node_iter = node_iter->next;
    }

    struct inode_t *node = malloc(sizeof(struct inode_t));

    node->owner = owner;
    node->owner_group = owner_group;
    node->mode = mode;
    node->stat.st_size = st_size;
    node->stat.st_blksize = st_blksize;
    node->stat.st_atim = st_atim;
    node->stat.st_mtim = st_mtim;
    node->stat.st_ctim = st_ctim;
    strcpy(node->filename, filename);
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

    return node->index;
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
        syslog(LOG_ERR, "service_create() - Node file descriptor corrupted");
        exit(EXIT_FAILURE);
    }

    struct inode_t *node = head, *prev;

    fseek(inode_file, 0, SEEK_SET);

    while (node) {
        fprintf(inode_file, "%d,%d,%d,%ld,%ld,%d,%ld,%ld,%ld,%s\n", node->index, node->owner, node->owner_group,
                node->mode, node->stat.st_size, node->stat.st_blksize, node->stat.st_atim.tv_sec,
                node->stat.st_mtim.tv_sec, node->stat.st_ctim.tv_sec, node->filename);

        prev = node;
        node = node->next;
        free(prev);
    }

    fclose(inode_file);
    return 0;
}

int remove_inode(struct inode_t **head, const char *name) {
    struct inode_t *node_iter = *head;
    while (node_iter) {
        if (strcmp(node_iter->filename, name) == 0) {
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