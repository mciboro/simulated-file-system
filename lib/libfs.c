#include "libfs.h"

static unsigned request_couter = 0;

static unsigned get_request_seq() { return request_couter++; }

fd_type libfs_create(char *const name, long mode) {
    int msgid = 0;
    unsigned copy_offset = 0;
    struct request_t *msg = malloc(sizeof(struct request_t) + sizeof(mode) + strlen(name));

    msgid = msgget(IPC_REQUESTS_KEY, IPC_PERMS);
    if (msgid == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue");
        exit(EXIT_FAILURE);
    }

    msg->type = CREATE;
    msg->seq = get_request_seq();
    msg->sender = getpid();
    msg->multipart = 0;
    msg->data_size = sizeof(mode) + strlen(name);
    msg->part_size = msg->data_size;
    msg->data_offset = 0;
    memcpy(msg->data + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(msg->data + copy_offset, name);

    if (msgsnd(msgid, msg, sizeof(*msg) + msg->data_size, 0) == -1) {
        fprintf(stderr, "libfs_create() - Failed to send message to queue");
        exit(EXIT_FAILURE);
    }

    free(msg);

    return 0;
}