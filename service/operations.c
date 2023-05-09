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
    long mode = 0;
    char name[req->data_size - sizeof(long) + 1];
    memcpy(&mode, req->data, sizeof(long));
    memcpy(name, req->data + sizeof(long), req->data_size - sizeof(long));
    memcpy(name + req->data_size - sizeof(long), "\0", 1);

    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "service_create() - Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    fd_type new_desc = creat(strcat(strcat(pw->pw_dir, "/libfs/"), name), mode);

    struct response_t *msg = malloc(sizeof(struct response_t) + sizeof(fd_type));
    msg->seq = req->seq;
    if (new_desc > 0) {
        msg->status = SUCCESS;
    } else {
        msg->status = FAILURE;
    }
    msg->multipart = false;
    msg->data_size = sizeof(fd_type);
    msg->data_offset = 0;
    msg->part_size = msg->data_size;
    memcpy(msg->data, &new_desc, sizeof(fd_type));

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size, 0) == -1) {
        syslog(LOG_ERR, "service_create() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
}