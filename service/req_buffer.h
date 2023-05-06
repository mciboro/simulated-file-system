#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

enum ReqStatus { COMPLETED, IN_PROGRESS, READY };

/* For communication between service threads */
struct service_req_t {
    pid_t sender;
    enum Type type;
    enum ReqStatus req_status;
    unsigned data_size;
    unsigned data_offset;
    char seq[16];
    char *data;
};

struct req_buffer_t {
    pthread_mutex_t lock;
    sem_t empty;
    sem_t full;
    unsigned wr_idx;
    unsigned rd_idx;
    unsigned size;
    unsigned struct_len;
    struct service_req_t reqs[BUFFER_SIZE];
};

int req_buffer_create(struct req_buffer_t **_rbuf, unsigned const size);
int req_bufer_destroy(struct req_buffer_t **_rbuf);
int handle_msg(struct req_buffer_t *rbuf, const struct request_t *msg);
int get_service_req(struct req_buffer_t *const rbuf, struct service_req_t *const data);