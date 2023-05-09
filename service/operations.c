#include "operations.h"

int service_create(struct service_req_t *req) {
    syslog(LOG_INFO, "Handling CREATE operation.");

    int msgid = 0;
    msgid = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);
    if (msgid == -1) {
        syslog(LOG_ERR, "service_create() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    //Code of operation on file

    struct response_t *msg = malloc(sizeof(struct response_t));

    msg->seq = req->seq;
    msg->status = SUCCESS;
    msg->multipart = false;
    msg->data_size = 0;
    msg->data_offset = 0;
    msg->part_size = 0;
    strcpy(msg->data, "");

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size, 0) == -1) {
        syslog(LOG_ERR, "service_create() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
}