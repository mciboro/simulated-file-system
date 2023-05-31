#include "operations.h"

int service_create(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling CREATE operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_create() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct create_args_t {
        uid_t file_owner;
        gid_t file_group;
        long access_mode;
        char filename[256];
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.access_mode = *(long *)(req->data + copy_off);
    copy_off += sizeof(long);
    strcpy(args.filename, req->data + copy_off);

    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME, &curr_time);
    fd_type node_index = 0;

    int status = 0;
    if (check_if_filename_taken(filename_table, args.filename) == 0) {
        status = add_inode(&inode_table, &node_index, F_REGULAR, args.file_owner, args.file_group, 1, args.access_mode, 0,
                               curr_time, curr_time, curr_time);
        if (add_filename_to_table(&filename_table, args.filename, node_index) == -1) {
            status = -1;
        }
    } else {
        status = -1;
    }

    unsigned data_size = 0, data_to_copy = 0;
    data_size = data_to_copy = sizeof(fd_type);
    char copy_buf[data_size];
    memcpy(copy_buf, &node_index, sizeof(fd_type));

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE + data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }

    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;

        struct response_t *msg = malloc(sizeof(struct response_t) + part_data_size);
        memset(msg, 0, sizeof(struct response_t) + part_data_size);
        msg->seq = req->seq;
        if (status == 0) {
            msg->status = SUCCESS;
        } else if (status == -1) {
            msg->status = FILENAME_TAKEN;
        } else {
            msg->status = FAILURE;
        }
        msg->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        msg->data_size = data_size;
        msg->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        msg->part_size = part_data_size;
        memcpy(msg->data, copy_buf + msg->data_offset, msg->part_size);

        if (msgsnd(msgid, msg, sizeof(*msg) + msg->part_size - sizeof(long), 0) == -1) {
            syslog(LOG_ERR, "service_create() - Failed to send message to queue");
            exit(EXIT_FAILURE);
        }

        free(msg);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    return status;
}

int service_rename(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling RENAME operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_create() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct rename_args_t {
        char oldname[256];
        char newname[256];
    } args;

    strcpy(args.oldname, req->data);
    strcpy(args.newname, req->data + strlen(args.oldname) + 1);

    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "service_create() - Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    int status = rename_file(filename_table, args.oldname, args.newname);

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    if (status == 0) {
        msg->status = SUCCESS;
    } else if (status == -2) {
        msg->status = FILENAME_TAKEN;
    } else {
        msg->status = FAILURE;
    }
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = 0;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_create() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_chmode(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling CHMODE operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_chmode() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct chmode_args_t {
        uid_t file_owner;
        gid_t file_group;
        long access_mode;
        char filename[256];
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.access_mode = *(long *)(req->data + copy_off);
    copy_off += sizeof(long);
    strcpy(args.filename, req->data + copy_off);

    uid_t file_owner = 0;
    gid_t file_group = 0;
    int status = 0;
    if (status = get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_chmode() - Can't find file owner and group");
    } else if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_chmode() - File owner or group doesn't match");
        status = FAILURE;
    } else if (status = chmod_inode(inode_table, args.filename, args.access_mode)) {
        syslog(LOG_ERR, "service_chmode() - Can't change access mode of file");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_chmode() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_stat(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling STAT operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_stat() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct stat_args_t {
        uid_t file_owner;
        gid_t file_group;
        char filename[256];
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    strcpy(args.filename, req->data + copy_off);

    uid_t file_owner = 0;
    gid_t file_group = 0;
    int status = 0;
    if (get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_stat() - Can't find file owner and group");
        status = FAILURE;
    }

    if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_stat() - File owner or group doesn't match");
        status = FILE_NOT_FOUND;
    }

    unsigned data_size = 0, data_to_copy = 0;
    data_size = data_to_copy = sizeof(struct stat_t);
    char *copy_buf[data_size];

    if (get_file_stat_from_inode(inode_table, args.filename, (struct stat_t *)&copy_buf) == -1) {
        data_to_copy = data_size = 0;
    }

    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE + data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }

    for (int i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;

        struct response_t *msg = malloc(sizeof(struct response_t) + part_data_size);
        memset(msg, 0, sizeof(struct response_t) + part_data_size);
        msg->seq = req->seq;
        msg->status = status;
        msg->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        msg->data_size = data_size;
        msg->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        msg->part_size = part_data_size;
        memcpy(msg->data, copy_buf + msg->data_offset, msg->part_size);

        if (msgsnd(msgid, msg, sizeof(*msg) + msg->part_size - sizeof(long), 0) == -1) {
            syslog(LOG_ERR, "service_stat() - Failed to send message to queue");
            exit(EXIT_FAILURE);
        }

        free(msg);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }

    return status;
}

int service_link(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling LINK operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_link() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct link_args_t {
        uid_t file_owner;
        gid_t file_group;
        char filename[256];
        char linkname[256];
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    strcpy(args.filename, req->data + copy_off);
    copy_off += strlen(args.filename) + 1;
    strcpy(args.linkname, req->data + copy_off);

    uid_t file_owner = 0;
    gid_t file_group = 0;
    int status = 0;
    if (get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_link() - Can't find file owner and group");
        status = -1;
    } else if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_link() - File owner or group doesn't match");
        status = -2;
    } else if ((status = create_hard_link(inode_table, args.filename, args.linkname)) != 0) {
        syslog(LOG_ERR, "service_link() - Can't create hard link");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_link() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_symlink(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling SYMLINK operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_symlink() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct symlink_args_t {
        uid_t file_owner;
        gid_t file_group;
        long mode;
        char filename[256];
        char linkname[256];
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.mode = *(long *)(req->data + copy_off);
    copy_off += sizeof(long);
    strcpy(args.filename, req->data + copy_off);
    copy_off += strlen(args.filename) + 1;
    strcpy(args.linkname, req->data + copy_off);

    syslog(LOG_INFO, "service_symlink() - file_owner: %d, file_group: %d, mode: %ld, filename: %s, linkname: %s", args.file_owner, args.file_group, args.mode, args.filename, args.linkname );

    uid_t file_owner = 0;
    gid_t file_group = 0;
    int status = 0;
    if (get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_symlink() - Can't find file owner and group");
        status = -1;
    } else if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_symlink() - File owner or group doesn't match");
        status = -2;
    } else if ((status = create_soft_link(inode_table, args.filename, args.linkname, args.file_owner, args.file_group, args.mode)) != 0) {
        syslog(LOG_ERR, "service_symlink() - Can't create symbolic link");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_symlink() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_open(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling OPEN operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_open() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct open_args_t {
        uid_t file_owner;
        gid_t file_group;
        char filename[256];
        int flags;
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    strcpy(args.filename, req->data + copy_off);
    copy_off += strlen(args.filename) + 1;
    args.flags = *(int *)(req->data + copy_off);

    syslog(LOG_INFO, "service_open() - file_owner: %d, file_group: %d, filename: %s, flags: %d", args.file_owner, args.file_group, args.filename, args.flags);

    uid_t file_owner = 0;
    gid_t file_group = 0;
    int status = 0;
    fd_type fd = 0;
    if (get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_open() - Can't find file owner and group");
        status = -1;
    } else if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_open() - File owner or group doesn't match");
        status = -2;
    } else if ((status = open_inode(inode_table, &fd, args.filename, args.flags)) != 0) {
        syslog(LOG_ERR, "service_open() - Can't open file");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = sizeof(fd_type);
    msg->data_offset = 0;
    msg->part_size = msg->data_size;
    memcpy(msg->data, &fd, sizeof(fd_type));

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_open() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_close(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling CLOSE operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_close() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct close_args_t {
        uid_t file_owner;
        gid_t file_group;
        fd_type fd;
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.fd = *(fd_type *)(req->data + copy_off);

    syslog(LOG_INFO, "service_close() - file_owner: %d, file_group: %d, fd: %d", args.file_owner, args.file_group, args.fd);

    int status = 0;
    if ((status = close_inode(args.fd)) != 0) {
        syslog(LOG_ERR, "service_close() - Can't close file");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_close() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_write(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling WRITE operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_write() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct write_args_t {
        uid_t file_owner;
        gid_t file_group;
        fd_type fd;
        unsigned int size;
        char *data;
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.fd = *(fd_type *)(req->data + copy_off);
    copy_off += sizeof(fd_type);
    args.size = *(unsigned int *)(req->data + copy_off);
    copy_off += sizeof(unsigned int);

    args.data = malloc(args.size);
    memcpy(args.data, req->data + copy_off, args.size);

    syslog(LOG_INFO, "service_write() - file_owner: %d, file_group: %d, fd: %d, size: %d", args.file_owner, args.file_group, args.fd, args.size);

    int status = 0;
    if ((status = write_inode_fd(inode_table, args.fd, args.data, args.size)) != 0) {
        syslog(LOG_ERR, "service_write() - Can't write to file");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_write() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_read(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling READ operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_read() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct read_args_t {
        uid_t file_owner;
        gid_t file_group;
        fd_type fd;
        unsigned int size;
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.fd = *(fd_type *)(req->data + copy_off);
    copy_off += sizeof(fd_type);
    args.size = *(unsigned int *)(req->data + copy_off);

    syslog(LOG_INFO, "service_read() - file_owner: %d, file_group: %d, fd: %d, size: %d", args.file_owner, args.file_group, args.fd, args.size);

    int status = 0;
    unsigned data_size = args.size, data_to_copy = args.size;
    char *copy_buf = malloc(args.size);
    if ((status = read_inode_fd(inode_table, args.fd, copy_buf, args.size)) != 0) {
        syslog(LOG_ERR, "service_read() - Can't write to file");
        data_size = data_to_copy = 0;
    }
    unsigned num_of_parts = data_size / MAX_MSG_DATA_SIZE + data_size % MAX_MSG_DATA_SIZE ? 1 : 0;
    if (num_of_parts == 0) {
        num_of_parts = 1;
    }
    for (unsigned i = 0; i < num_of_parts; i++) {
        unsigned part_data_size = data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
        struct response_t *msg = malloc(sizeof(struct response_t) + part_data_size);
        memset(msg, 0, sizeof(struct response_t) + part_data_size);
        msg->seq = req->seq;
        msg->status = status;
        msg->multipart = num_of_parts == 1 ? 0 : num_of_parts;
        msg->data_size = data_size;
        msg->data_offset = i ? i * MAX_MSG_DATA_SIZE : 0;
        msg->part_size = part_data_size;
        memcpy(msg->data, copy_buf + msg->data_offset, msg->part_size);

        if (msgsnd(msgid, msg, sizeof(*msg) + msg->part_size - sizeof(long), 0) == -1) {
            syslog(LOG_ERR, "service_stat() - Failed to send message to queue");
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO, "service_read() - Sent part %d of %d", i + 1, num_of_parts);
        free(msg);
        data_to_copy -= data_to_copy > MAX_MSG_DATA_SIZE ? MAX_MSG_DATA_SIZE : data_to_copy;
    }
    free(copy_buf);
    syslog(LOG_INFO, "service_read() - Sent all parts");
    return status;
}

int service_seek(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling SEEK operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_seek() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct seek_args_t {
        uid_t file_owner;
        gid_t file_group;
        fd_type fd;
        unsigned offset;
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    args.fd = *(fd_type *)(req->data + copy_off);
    copy_off += sizeof(fd_type);
    args.offset = *(unsigned *)(req->data + copy_off);

    syslog(LOG_INFO, "service_seek() - file_owner: %d, file_group: %d, fd: %d, offset: %d", args.file_owner, args.file_group, args.fd, args.offset);

    int status = 0;
    if ((status = seek_inode_fd(inode_table, args.fd, args.offset)) != 0) {
        syslog(LOG_ERR, "service_seek() - Can't seek file");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_seek() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}

int service_unlink(struct service_req_t *req)
{
    syslog(LOG_INFO, "Handling UNLINK operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_unlink() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    // Code of operation on file
    struct unlink_args_t {
        uid_t file_owner;
        gid_t file_group;
        char name[256];
    } args;

    unsigned copy_off = 0;
    args.file_owner = *(uid_t *)(req->data + copy_off);
    copy_off += sizeof(uid_t);
    args.file_group = *(gid_t *)(req->data + copy_off);
    copy_off += sizeof(gid_t);
    strcpy(args.name, req->data + copy_off);

    syslog(LOG_INFO, "service_unlink() - file_owner: %d, file_group: %d, name: %s", args.file_owner, args.file_group, args.name);

    int status = 0;
    if ((status = unlink_inode(inode_table, args.name)) != 0) {
        syslog(LOG_ERR, "service_unlink() - Can't unlink file");
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = status;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_unlink() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return status;
}