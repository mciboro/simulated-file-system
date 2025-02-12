#include "libfs.h"

unsigned seq_counter = 0;
int libfs_errno = 0;

long get_seq() { return (getpid() << sizeof(long) / 2) + seq_counter++; }

fd_type libfs_create(char *const name, long mode) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + sizeof(mode) + strlen(name) + 1;

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(copy_buf + copy_offset, name);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = CREATE;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_create() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(create_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *create_resp = malloc(MAX_MSG_SIZE);
    memset(create_resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, create_resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = create_resp->status;
    unsigned resp_data_size = create_resp->data_size;
    unsigned copied_data = create_resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, create_resp->data, copied_data);
    free(create_resp);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE, seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s created successfully. New desc: %d.\n", name, *(fd_type *)resp_buf);
    } else if (status == FILENAME_TAKEN) {
        libfs_errno = FILENAME_TAKEN;
        fprintf(stderr, "Filename %s has been already taken.\n", name);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "File %s wasn't created!\n", name);
    }

    fd_type new_desc = *(fd_type *)resp_buf;

    return new_desc;
}

int libfs_rename(const char *oldname, const char *newname) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = strlen(oldname) + 1 + strlen(newname) + 1;

    char copy_buf[data_size];
    strcpy(copy_buf + copy_offset, oldname);
    copy_offset += strlen(oldname) + 1;
    strcpy(copy_buf + copy_offset, newname);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *rename_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(rename_req, 0, sizeof(struct request_t) + part_data_size);

        rename_req->type = RENAME;
        rename_req->seq = seq;
        rename_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        rename_req->data_size = data_size;
        rename_req->part_size = part_data_size;
        rename_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(rename_req->data, copy_buf + rename_req->data_offset, rename_req->part_size);

        if (msgsnd(request_queue, rename_req, sizeof(struct request_t) + rename_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_rename() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(rename_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *rename_resp = malloc(sizeof(struct response_t));
    memset(rename_resp, 0, sizeof(struct response_t));

    int msg_len = 0, status = 0;
    msgrcv(response_queue, rename_resp, sizeof(struct response_t) - sizeof(long), seq, 0);
    msg_len = sizeof(*rename_resp) + rename_resp->part_size;
    status = rename_resp->status;

    if (msg_len <= 0) {
        syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
        exit(EXIT_FAILURE);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s changed name successfully to %s.\n", oldname, newname);
    } else if (status == FILENAME_TAKEN) {
        libfs_errno = FILENAME_TAKEN;
        fprintf(stderr, "Filename %s has already been taken.\n", newname);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Filename wasn't changed!\n");
    }

    free(rename_resp);

    return status;
}

int libfs_chmode(char *const name, long mode) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_chmode() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + sizeof(mode) + strlen(name) + 1;

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(copy_buf + copy_offset, name);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = CHMODE;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_chmode() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(create_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(sizeof(struct response_t));

    int msg_len = 0, status = 0;
    msgrcv(response_queue, resp, sizeof(struct response_t) - sizeof(long), seq, 0);
    msg_len = sizeof(*resp) + resp->part_size;
    status = resp->status;

    if (msg_len <= 0) {
        syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
        exit(EXIT_FAILURE);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s mode changed successfully.\n", name);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File %s not found.\n", name);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "File %s mode wasn't changed!\n", name);
    }

    free(resp);

    return status;
}

int libfs_stat(const char *path, struct stat_t *buf) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_stat() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + strlen(path) + 1;

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    strcpy(copy_buf + copy_offset, path);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = STAT;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_stat() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(create_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    unsigned resp_data_size = resp->data_size;
    unsigned copied_data = resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, resp->data, copied_data);
    free(resp);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE, seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }

    if (status == SUCCESS) {
        memcpy(buf, resp_buf, sizeof(struct stat_t));
        struct stat_t *stat = (struct stat_t *)resp_buf;
        fprintf(stderr, "File %s stat: st_size: %ld, st_atime: %ld, st_mtime: %ld, st_ctime: %ld\n", path,
                stat->st_size, stat->st_atim.tv_sec, stat->st_mtim.tv_sec, stat->st_ctim.tv_sec);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File %s not found!\n", path);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Unable to get file stat for file %s!\n", path);
    }

    return status;
}

int libfs_link(const char *oldpath, const char *newpath) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_link() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + strlen(oldpath) + 1 + strlen(newpath) + 1;

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    strcpy(copy_buf + copy_offset, oldpath);
    copy_offset += strlen(oldpath) + 1;
    strcpy(copy_buf + copy_offset, newpath);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = LINK;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_link() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(create_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    unsigned resp_data_size = resp->data_size;
    unsigned copied_data = resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, resp->data, copied_data);
    free(resp);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE, seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "Link %s -> %s created!\n", oldpath, newpath);
    } else if (status == FILENAME_TAKEN) {
        libfs_errno = FILENAME_TAKEN;
        fprintf(stderr, "Link %s -> %s failed! Target already exists!\n", oldpath, newpath);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "Link %s -> %s failed! File %s not found!\n", oldpath, newpath, oldpath);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Link %s -> %s failed!\n", oldpath, newpath);
    }

    return status;
}

int libfs_symlink(const char *path1, const char *path2, long mode) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_symlink() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + sizeof(long) + strlen(path1) + 1 + strlen(path2) + 1;

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(copy_buf + copy_offset, path1);
    copy_offset += strlen(path1) + 1;
    strcpy(copy_buf + copy_offset, path2);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = SYMLINK;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_symlink() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(create_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    unsigned resp_data_size = resp->data_size;
    unsigned copied_data = resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, resp->data, copied_data);
    free(resp);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE, seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "Symlink %s -> %s created!\n", path1, path2);
    } else if (status == FILENAME_TAKEN) {
        libfs_errno = FILENAME_TAKEN;
        fprintf(stderr, "Symlink %s -> %s failed! Target already exists!\n", path1, path2);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "Symlink %s -> %s failed! File %s not found!\n", path1, path2, path1);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Symlink %s -> %s failed!\n", path1, path2);
    }
    return status;
}

fd_type libfs_open(char *const name, const int flags) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_open() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + strlen(name) + 1 + sizeof(flags);

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    strcpy(copy_buf + copy_offset, name);
    copy_offset += strlen(name) + 1;
    memcpy(copy_buf + copy_offset, &flags, sizeof(flags));

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = OPEN;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_open() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        data_to_copy -= part_data_size;
        free(create_req);
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    unsigned resp_data_size = resp->data_size;
    unsigned copied_data = resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, resp->data, copied_data);
    free(resp);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE, seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s opened with descriptor %d!\n", name, resp_buf[0]);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File %s not found!\n", name);
    } else if (status == INSUFFICIENT_PERMS) {
        libfs_errno = INSUFFICIENT_PERMS;
        fprintf(stderr, "Insufficient permissions. Access denied!\n");
    } else if (status == DOUBLE_WRITE) {
        libfs_errno = DOUBLE_WRITE;
        fprintf(stderr, "File %s has already been opened for writing!\n", name);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "File %s open failed!\n", name);
    }

    return status == SUCCESS ? resp_buf[0] : -1;
}

int libfs_close(const fd_type fd) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_close() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + sizeof(fd);

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &fd, sizeof(fd));

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = CLOSE;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_close() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        data_to_copy -= part_data_size;
        free(create_req);
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    unsigned resp_data_size = resp->data_size;
    unsigned copied_data = resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, resp->data, copied_data);
    free(resp);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE, seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File descriptor %d closed!\n", fd);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File descriptor %d not found!\n", fd);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "File descriptor %d close failed!\n", fd);
    }

    return status == SUCCESS ? 0 : -1;
}

int libfs_write(const fd_type fd, const char *buf, const unsigned int size) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_write() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + sizeof(fd) + sizeof(size) + size;

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &fd, sizeof(fd));
    copy_offset += sizeof(fd);
    memcpy(copy_buf + copy_offset, &size, sizeof(size));
    copy_offset += sizeof(size);
    memcpy(copy_buf + copy_offset, buf, size);

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE + (data_size % MAX_MSG_DATA_SIZE != 0 ? 1 : 0);
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (unsigned i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = WRITE;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_write() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        data_to_copy -= part_data_size;
        free(create_req);
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (response_queue == -1) {
        fprintf(stderr, "libfs_write() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    free(resp);

    if (status == SUCCESS) {
        fprintf(stderr, "File descriptor %d write success!\n", fd);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File descriptor %d not found!\n", fd);
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "File descriptor %d write failed!\n", fd);
    }

    return status == SUCCESS ? 0 : -1;
}

int libfs_read(const fd_type fd, char *buf, const unsigned int size) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_read() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_to_copy = 0, data_size = 0;
    data_size = data_to_copy = sizeof(uid) + sizeof(gid) + sizeof(fd) + sizeof(size);

    char copy_buf[data_size];
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &fd, sizeof(fd));
    copy_offset += sizeof(fd);
    memcpy(copy_buf + copy_offset, &size, sizeof(size));

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE;
    num_of_parts += data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct request_t *create_req = malloc(sizeof(struct request_t) + part_data_size);
        memset(create_req, 0, sizeof(struct request_t) + part_data_size);

        create_req->type = READ;
        create_req->seq = seq;
        create_req->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        create_req->data_size = data_size;
        create_req->part_size = part_data_size;
        create_req->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        memcpy(create_req->data, copy_buf + create_req->data_offset, create_req->part_size);

        if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) ==
            -1) {
            fprintf(stderr, "libfs_read() - Failed to send message to queue\n");
            exit(EXIT_FAILURE);
        }

        free(create_req);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }
    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;
    unsigned resp_data_size = resp->data_size;
    unsigned copied_data = resp->part_size;
    char resp_buf[resp_data_size];

    memcpy(resp_buf, resp->data, copied_data);

    while (copied_data < resp_data_size) {
        struct response_t *further_resp = malloc(MAX_MSG_SIZE);
        memset(further_resp, 0, MAX_MSG_SIZE);

        if (msgrcv(response_queue, further_resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
            syslog(LOG_ERR, "Error in msgrcv()");
            exit(EXIT_FAILURE);
        }


        if (!further_resp->multipart) {
            syslog(LOG_ERR, "Error in msgrcv() - Msg should be multipart");
            exit(EXIT_FAILURE);
        }

        memcpy(resp_buf + further_resp->data_offset, further_resp->data, further_resp->part_size);
        copied_data += further_resp->part_size;

        status = further_resp->status;
        free(further_resp);
    }
    fprintf(stderr, "Read data: ");
    for (int i = 0; i < resp_data_size; i++) {
        fprintf(stderr, "%c", resp_buf[i]);
    }
    fprintf(stderr, "\n");
    if (status == SUCCESS) {
        memcpy(buf, &resp_buf, resp->data_size);
        fprintf(stderr, "File descriptor %d read success!\n", fd);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File not found!\n");
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Unable to read!\n");
    }

    free(resp);
    return status;
}

int libfs_seek(const fd_type fd, const unsigned offset) {
    int request_queue = 0, response_queue = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_seek() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_size = sizeof(uid) + sizeof(gid) + sizeof(fd) + sizeof(offset);

    char copy_buf[data_size];
    unsigned copy_offset = 0;
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, &fd, sizeof(fd));
    copy_offset += sizeof(fd);
    memcpy(copy_buf + copy_offset, &offset, sizeof(offset));

    struct request_t *create_req = malloc(sizeof(struct request_t) + data_size);
    memset(create_req, 0, sizeof(struct request_t) + data_size);

    create_req->type = SEEK;
    create_req->seq = seq;
    create_req->multipart = 0;
    create_req->data_size = data_size;
    create_req->part_size = data_size;
    create_req->data_offset = 0;
    memcpy(create_req->data, copy_buf, create_req->part_size);

    if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) == -1) {
        fprintf(stderr, "libfs_seek() - Failed to send message to queue\n");
        exit(EXIT_FAILURE);
    }

    free(create_req);

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;

    if (status == SUCCESS) {
        fprintf(stderr, "File descriptor %d seek success!\n", fd);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File not found!\n");
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Unable to seek!\n");
    }

    free(resp);
    return status;
}

int libfs_unlink(char *const name) {
    int request_queue = 0, response_queue = 0;

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_unlink() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    unsigned seq = get_seq();

    unsigned data_size = sizeof(uid) + sizeof(gid) + strlen(name) + 1;

    char copy_buf[data_size];
    unsigned copy_offset = 0;
    memcpy(copy_buf + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(copy_buf + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(copy_buf + copy_offset, name, strlen(name) + 1);

    struct request_t *create_req = malloc(sizeof(struct request_t) + data_size);
    memset(create_req, 0, sizeof(struct request_t) + data_size);

    create_req->type = UNLINK;
    create_req->seq = seq;
    create_req->multipart = 0;
    create_req->data_size = data_size;
    create_req->part_size = data_size;
    create_req->data_offset = 0;
    memcpy(create_req->data, copy_buf, create_req->part_size);

    if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) == -1) {
        fprintf(stderr, "libfs_unlink() - Failed to send message to queue\n");
        exit(EXIT_FAILURE);
    }

    free(create_req);

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(MAX_MSG_SIZE);
    memset(resp, 0, MAX_MSG_SIZE);

    if (msgrcv(response_queue, resp, MAX_MSG_SIZE - sizeof(long), seq, 0) == -1) {
        syslog(LOG_ERR, "Error in msgrcv()");
        exit(EXIT_FAILURE);
    }

    int status = resp->status;

    if (status == SUCCESS) {
        fprintf(stderr, "File %s unlinked!\n", name);
    } else if (status == FILE_NOT_FOUND) {
        libfs_errno = FILE_NOT_FOUND;
        fprintf(stderr, "File not found!\n");
    } else {
        libfs_errno = FAILURE;
        fprintf(stderr, "Unable to unlink!\n");
    }

    free(resp);

    return status;
}