#include "libfs.h"

unsigned seq_counter = 0;

fd_type libfs_create(char *const name, long mode) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;
    struct request_t *req = malloc(sizeof(struct request_t) + sizeof(mode) + strlen(name));

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    req->type = CREATE;
    req->seq = get_seq();
    req->multipart = 0;
    req->data_size = sizeof(mode) + strlen(name);
    req->part_size = req->data_size;
    req->data_offset = 0;
    memcpy(req->data + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(req->data + copy_offset, name);

    if (msgsnd(request_queue, req, sizeof(*req) + req->data_size, 0) == -1) {
        fprintf(stderr, "libfs_create() - Failed to send message to queue\n");
        exit(EXIT_FAILURE);
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *resp = malloc(sizeof(struct response_t) + sizeof(fd_type));
    int msg_len = 0, status = 0;
    msgrcv(response_queue, resp, sizeof(struct response_t) + sizeof(fd_type), req->seq, 0);
    msg_len = sizeof(*resp) + resp->part_size;
    status = resp->status;

    if (msg_len <= 0) {
        syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
        exit(EXIT_FAILURE);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s created successfully. New desc: %d\n", name, *(fd_type*)resp->data);
    } else {
        fprintf(stderr, "File %s wasn't created!\n", name);
    }

    free(req);
    free(resp);

    return *(fd_type*)resp->data;
}