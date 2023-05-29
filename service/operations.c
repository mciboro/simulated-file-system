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
        status = add_inode(&inode_table, &node_index, args.file_owner, args.file_group, 1, args.access_mode, 0,
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
    char copy_buf[data_size];

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