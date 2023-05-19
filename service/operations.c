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

    args = *(struct create_args_t *)req->data;

    // uid_t uid = getuid();
    // struct passwd *pw = getpwuid(uid);

    // if (pw == NULL) {
    //     syslog(LOG_ERR, "service_create() - Cannot retrieve info about service's owner");
    //     exit(EXIT_FAILURE);
    // }

    // fd_type new_desc = creat(strcat(strcat(pw->pw_dir, "/libfs/"), args.filename), 0700);
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME, &curr_time);
    fd_type node_index = add_inode(&inode_table, args.filename, args.file_owner, args.file_group, args.access_mode, 0,
                                   curr_time, curr_time, curr_time);

    struct response_t *msg = malloc(sizeof(struct response_t) + sizeof(fd_type));
    memset(msg, 0, sizeof(struct response_t) + sizeof(fd_type));
    msg->seq = req->seq;
    if (node_index >= 0) {
        msg->status = SUCCESS;
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
    } else {
        msg->status = FAILURE;
    }
    msg->multipart = false;
    msg->data_size = sizeof(fd_type);
    msg->data_offset = 0;
    msg->part_size = 0;

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size - sizeof(long), 0) == -1) {
        syslog(LOG_ERR, "service_create() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
}