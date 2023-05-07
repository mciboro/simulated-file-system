#include "libfs.h"

unsigned seq_counter = 0;

fd_type libfs_create(char *const name, long mode) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;
    struct request_t *msg = malloc(sizeof(struct request_t) + sizeof(mode) + strlen(name));

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    msg->type = CREATE;
    msg->sender = getpid();
    msg->seq = (msg->sender << sizeof(long)/2) + seq_counter++;
    msg->multipart = 0;
    msg->data_size = sizeof(mode) + strlen(name);
    msg->part_size = msg->data_size;
    msg->data_offset = 0;
    memcpy(msg->data + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(msg->data + copy_offset, name);

    if (msgsnd(request_queue, msg, sizeof(*msg) + msg->data_size, 0) == -1) {
        fprintf(stderr, "libfs_create() - Failed to send message to queue\n");
        exit(EXIT_FAILURE);
    }

    free(msg);

    // response_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);

    // if (response_queue == -1) {
    //     syslog(LOG_ERR, "Error in msgget()");
    //     exit(EXIT_FAILURE);
    // }

    // struct request_t msg = {0};
    // int msg_len = 0;
    // while (working) {
    //     msgrcv(response_queue, &msg, MAX_MSG_SIZE, 0, 0);
    //     msg_len = sizeof(msg) + msg.part_size;

    //     if (msg_len <= 0) {
    //         syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
    //         exit(EXIT_FAILURE);
    //     }

    //     syslog(LOG_INFO, "Msg len: %d", msg_len);

    //     if (handle_msg(server_buf, &msg)) {
    //         syslog(LOG_ERR, "msgrcv() - Message handling failed!");
    //         exit(EXIT_FAILURE);
    //     }
    // }

    return 0;
}