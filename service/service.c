#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "service.h"

#define MAX_MSG_SIZE 8192

volatile sig_atomic_t working = true;
struct req_buffer_t *server_buf = NULL;

void end_service() { working = false; }

void *receive(void *service_args) {
    int msgid = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);

    if (msgid == -1) {
        perror("Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct request_t req_msg = {0};
    int msg_len = 0;
    while (working) {
        msg_len = msgrcv(msgid, &req_msg, 0, 0, IPC_NOWAIT);
        if (msg_len == -1 && errno == ENOMSG) {
            sleep(1);
            continue;
        } else if (msg_len == -1) {
            perror("Error in msgrcv()");
            exit(EXIT_FAILURE);
        }

        if (msg_len > 0 && msg_len <= MAX_MSG_SIZE) {
            msg_len = msgrcv(msgid, &req_msg, msg_len, 0, IPC_NOWAIT);
            if (msg_len == -1) {
                perror("Error in msgrcv()");
                exit(EXIT_FAILURE);
            }
        } else {
            syslog(LOG_ERR, "Message length is incorrect: %d\n", msg_len);
        }

        if (handle_msg(server_buf, &req_msg)) {
            syslog(LOG_ERR, "Message handling failed!\n");
            exit(EXIT_FAILURE);
        }
    }
}

void *operate(void *worker_args) {
    int status = 0;

    while (working) {
        struct service_req_t req_obj = {0};
        struct service_req_t *req = &req_obj;

        status = get_service_req(server_buf, req);

        if (status) {
            syslog(LOG_ERR, "Data receiving failed!\n");
            exit(EXIT_FAILURE);
        }
        if (req->req_status != READY) {
            syslog(LOG_ERR, "Data is corrupted!\n");
            exit(EXIT_FAILURE);
        }

        switch (req->type) {
        case CREATE:
            /* code */
            break;
        // TO DO
        default:
            syslog(LOG_ERR, "Unrecognized type!\n");
            exit(EXIT_FAILURE);
        }

        free(req->data);
        req->data = NULL;
        req->data_offset = 0;
        req->data_size = 0;
        req->req_status = COMPLETED;
        req->sender = 0;
        req->seq = 0;

        req = NULL;
    }
}

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error in fork()");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    pid_t sid = setsid();

    if (sid < 0) {
        perror("Error in setsid()");
        exit(EXIT_FAILURE);
    }

    if (chdir("/") < 0) {
        perror("Error in chdir()");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    openlog("libfs-service", LOG_PID, LOG_DAEMON);

    struct sigaction sigint = {0}, sigterm = {0};
    sigint.sa_handler = end_service;
    sigterm.sa_handler = end_service;
    sigemptyset(&sigint.sa_mask);
    sigemptyset(&sigterm.sa_mask);
    sigint.sa_flags = 0;
    sigterm.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);
    sigaction(SIGTERM, &sigterm, NULL);

    req_buffer_create(&server_buf, BUFFER_SIZE);

    pthread_t receiver = {0}, worker = {0};
    pthread_create(&receiver, NULL, receive, NULL);
    pthread_create(&worker, NULL, operate, NULL);

    pthread_join(receiver, NULL);
    pthread_join(worker, NULL);

    req_bufer_destroy(&server_buf);

    closelog();
    exit(EXIT_SUCCESS);
}
