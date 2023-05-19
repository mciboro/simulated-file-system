#include "libfs.h"

unsigned seq_counter = 0;

fd_type libfs_create(char *const name, long mode) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;
    struct request_t *create_req =
        malloc(sizeof(struct request_t) + sizeof(uid_t) + sizeof(gid_t) + sizeof(long) + strlen(name) + 1);
    memset(create_req, 0, sizeof(struct request_t) + sizeof(uid_t) + sizeof(gid_t) + sizeof(long) + strlen(name) + 1);


    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    create_req->type = CREATE;
    create_req->seq = get_seq();
    create_req->multipart = 0;
    create_req->data_size = sizeof(uid) + sizeof(gid) + sizeof(mode) + strlen(name) + 1;
    create_req->part_size = create_req->data_size;
    create_req->data_offset = 0;

    memcpy(create_req->data + copy_offset, &uid, sizeof(uid));
    copy_offset += sizeof(uid);
    memcpy(create_req->data + copy_offset, &gid, sizeof(gid));
    copy_offset += sizeof(gid);
    memcpy(create_req->data + copy_offset, &mode, sizeof(mode));
    copy_offset += sizeof(mode);
    strcpy(create_req->data + copy_offset, name);

    if (msgsnd(request_queue, create_req, sizeof(struct request_t) + create_req->part_size - sizeof(long), 0) == -1) {
        fprintf(stderr, "libfs_create() - Failed to send message to queue\n");
        exit(EXIT_FAILURE);
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *create_resp = malloc(sizeof(struct response_t) + sizeof(fd_type));
    memset(create_resp, 0, sizeof(struct request_t) + sizeof(fd_type));

    int msg_len = 0, status = 0;
    msgrcv(response_queue, create_resp, sizeof(struct response_t) + sizeof(fd_type) - sizeof(long), create_req->seq, 0);
    msg_len = sizeof(*create_resp) + create_resp->part_size;
    status = create_resp->status;

    if (msg_len <= 0) {
        syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
        exit(EXIT_FAILURE);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s created successfully. New desc: %d\n", name, *(fd_type *)create_resp->data);
    } else {
        fprintf(stderr, "File %s wasn't created!\n", name);
    }

    fd_type new_desc = *(fd_type *)create_resp->data;

    free(create_req);
    free(create_resp);

    return new_desc;
}

int libfs_rename(const char *oldname, const char *newname) {
    int request_queue = 0, response_queue = 0;
    unsigned copy_offset = 0;
    struct request_t *rename_req = malloc(sizeof(struct request_t) + strlen(oldname) + 1 + strlen(newname) + 1);
    memset(rename_req, 0, sizeof(struct request_t) + strlen(oldname) + 1 + strlen(newname) + 1);

    request_queue = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);
    if (request_queue == -1) {
        fprintf(stderr, "libfs_create() - Failed to open message queue\n");
        exit(EXIT_FAILURE);
    }

    uid_t uid = getuid();
    gid_t gid = getgid();

    rename_req->type = RENAME;
    rename_req->seq = get_seq();
    rename_req->multipart = 0;
    rename_req->data_size = strlen(oldname) + 1 + strlen(newname) + 1;
    rename_req->part_size = rename_req->data_size;
    rename_req->data_offset = 0;

    strcpy(rename_req->data + copy_offset, oldname);
    copy_offset += strlen(oldname) + 1;
    strcpy(rename_req->data + copy_offset, newname);

    if (msgsnd(request_queue, rename_req, sizeof(struct request_t) + rename_req->part_size - sizeof(long), 0) == -1) {
        fprintf(stderr, "libfs_create() - Failed to send message to queue\n");
        exit(EXIT_FAILURE);
    }

    response_queue = msgget(IPC_RESPONSE_KEY, IPC_PERMS | IPC_CREAT);

    if (response_queue == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct response_t *rename_resp = malloc(sizeof(struct response_t));
    memset(rename_resp, 0, sizeof(struct response_t));

    int msg_len = 0, status = 0;
    msgrcv(response_queue, rename_resp, sizeof(struct response_t) - sizeof(long), rename_req->seq, 0);
    msg_len = sizeof(*rename_resp) + rename_resp->part_size;
    status = rename_resp->status;

    if (msg_len <= 0) {
        syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
        exit(EXIT_FAILURE);
    }

    if (status == SUCCESS) {
        fprintf(stderr, "File %s changed name successfully to %s\n", oldname, newname);
    } else {
        fprintf(stderr, "Filename wasn't created!\n");
    }

    free(rename_req);
    free(rename_resp);

    return status;
}