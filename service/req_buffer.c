#include "req_buffer.h"

int req_buffer_create(struct req_buffer_t **_rbuf, unsigned const size) {
    if (size > 0) {
        struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * size);
        memset(rbuf, 0, sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * size);

        rbuf->struct_len = sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * size;
        rbuf->size = size;
        rbuf->rd_idx = 0;
        rbuf->wr_idx = 0;
        sem_init(&rbuf->empty, 0, rbuf->size);
        sem_init(&rbuf->full, 0, 0);
        pthread_mutex_init(&rbuf->lock, NULL);
        *_rbuf = rbuf;
        return 0;
    } else {
        syslog(LOG_ERR, "Size of ring buffer must be non zero!");
        return -1;
    }
}

int req_bufer_destroy(struct req_buffer_t **_rbuf) {
    struct req_buffer_t *rbuf = *_rbuf;
    if (rbuf) {
        sem_destroy(&rbuf->empty);
        sem_destroy(&rbuf->full);
        pthread_mutex_destroy(&rbuf->lock);
        free(rbuf);
        *_rbuf = NULL;
        return 0;
    } else {
        syslog(LOG_ERR, "Memory already void!");
        return -1;
    }
}

int handle_msg(struct req_buffer_t *rbuf, const struct request_t *msg) {
    if (!rbuf) {
        syslog(LOG_ERR, "Ring buffer void!");
        return -1;
    }

    if (!msg->multipart) {
        sem_wait(&rbuf->empty);
        pthread_mutex_lock(&rbuf->lock);

        struct service_req_t new_req = {0};
        new_req.type = msg->type;
        new_req.seq = msg->seq;
        new_req.req_status = READY;
        new_req.data_size = msg->data_size;
        new_req.data = malloc(new_req.data_size);

        memcpy(new_req.data, msg->data, msg->data_size);
        rbuf->reqs[rbuf->wr_idx] = new_req;
        rbuf->wr_idx = (rbuf->wr_idx + 1) % rbuf->size;

        syslog(LOG_INFO, "Request SEQ: %ld added to buffer.", msg->seq);

        pthread_mutex_unlock(&rbuf->lock);
        sem_post(&rbuf->full);
    } else if (msg->multipart && msg->data_offset == 0) {
        sem_wait(&rbuf->empty);
        pthread_mutex_lock(&rbuf->lock);

        struct service_req_t new_req = {0};
        new_req.type = msg->type;
        new_req.seq = msg->seq;
        new_req.req_status = IN_PROGRESS;
        new_req.data_size = msg->data_size;
        new_req.data = malloc(new_req.data_size);

        rbuf->reqs[rbuf->wr_idx] = new_req;
        rbuf->wr_idx = (rbuf->wr_idx + 1) % rbuf->size;

        syslog(LOG_INFO, "First part of request SEQ: %ld added to buffer.", msg->seq);

        pthread_mutex_unlock(&rbuf->lock);
        sem_post(&rbuf->full);
    } else if (msg->multipart && msg->data_offset > 0) {
        for (int i = rbuf->rd_idx + 1; i < rbuf->wr_idx; i++) {
            if (rbuf->reqs[i].seq == msg->seq) {
                pthread_mutex_lock(&rbuf->lock);

                memcpy(rbuf->reqs[i].data + rbuf->reqs[i].data_offset, msg->data, msg->data_size);
                rbuf->reqs[i].data_offset += msg->data_size;

                if (rbuf->reqs[i].data_offset == rbuf->reqs[i].data_size) {
                    rbuf->reqs[i].req_status = READY;
                } else if (rbuf->reqs[i].data_offset > rbuf->reqs[i].data_size) {
                    syslog(LOG_ERR, "handle_msg() - Corrupted data size\n");
                    return -3;
                }

                syslog(LOG_INFO, "Next part of request SEQ: %ld added to buffer.", msg->seq);
                pthread_mutex_unlock(&rbuf->lock);
            }
        }

        syslog(LOG_ERR, "handle_msg() - No sequence in buffer\n");
        return -1;
    } else {
        syslog(LOG_ERR, "handle_msg() - Unhandled situation\n");
        return -2;
    }

    return 0;
}

int get_service_req(struct req_buffer_t *const rbuf, struct service_req_t **data) {
    if (!rbuf) {
        syslog(LOG_ERR, "Ring buffer void!");
        return -1;
    }

    if (!data) {
        syslog(LOG_ERR, "Data object cannot be void!");
        return -2;
    }

    sem_wait(&rbuf->full);
    pthread_mutex_lock(&rbuf->lock);
    *data = &rbuf->reqs[rbuf->rd_idx];
    rbuf->rd_idx = (rbuf->rd_idx + 1) % rbuf->size;
    pthread_mutex_unlock(&rbuf->lock);
    sem_post(&rbuf->empty);

    return 0;
}