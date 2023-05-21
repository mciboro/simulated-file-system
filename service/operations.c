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
    int status = add_inode(&inode_table, &node_index, args.filename, args.file_owner, args.file_group, args.access_mode,
                           0, curr_time, curr_time, curr_time);

    struct response_t *msg = malloc(sizeof(struct response_t) + sizeof(fd_type));
    memset(msg, 0, sizeof(struct response_t) + sizeof(fd_type));
    msg->seq = req->seq;
    if (status == 0) {
        msg->status = SUCCESS;
    } else if (status == -1) {
        msg->status = FILENAME_TAKEN;
    } else {
        msg->status = FAILURE;
    }
    msg->multipart = false;
    msg->data_size = sizeof(fd_type);
    msg->data_offset = 0;
    msg->part_size = msg->data_size;
    memcpy(msg->data, &node_index, sizeof(fd_type));

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_create() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
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

    int status = rename_inode(inode_table, args.oldname, args.newname);

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

    return 0;
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
    if (get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_chmode() - Can't find file owner and group");
        return -1;
    }

    if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_chmode() - File owner or group doesn't match");
        return -2;
    }

    if (chmod_inode(inode_table, args.filename, args.access_mode)) {
        syslog(LOG_ERR, "service_chmode() - Can't change access mode of file");
        return -3;
    }

    struct response_t *msg = malloc(sizeof(struct response_t));
    memset(msg, 0, sizeof(struct response_t));
    msg->seq = req->seq;
    msg->status = SUCCESS;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_chmode() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
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
    if (get_file_owner_and_group(inode_table, args.filename, &file_owner, &file_group)) {
        syslog(LOG_ERR, "service_stat() - Can't find file owner and group");
        return -1;
    }

    if (file_owner != args.file_owner || file_group != args.file_group) {
        syslog(LOG_ERR, "service_stat() - File owner or group doesn't match");
        exit(EXIT_FAILURE);
    }

    struct response_t *msg = malloc(sizeof(struct response_t) + sizeof(struct stat_t));
    memset(msg, 0, sizeof(struct response_t) + sizeof(struct stat_t));
    msg->seq = req->seq;
    msg->status = SUCCESS;
    msg->multipart = false;
    msg->data_size = sizeof(struct stat_t);
    msg->data_offset = 0;
    msg->part_size = msg->data_size;

    memset(msg->data, 0, sizeof(struct stat_t));
    struct stat_t *libfs_stat = (struct stat_t *)msg->data;
    get_file_stat_from_inode(inode_table, args.filename, libfs_stat);

    memcpy(msg->data, libfs_stat, sizeof(struct stat_t));

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_stat() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
}