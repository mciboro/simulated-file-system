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

volatile sig_atomic_t working = true;
struct req_buffer_t *server_buf = NULL;

void end_service() { working = false; }

void *receive(void *service_args) {
    int msgid = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);

    if (msgid == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    struct request_t msg = {0};
    int msg_len = 0;
    while (working) {
        msgrcv(msgid, &msg, MAX_MSG_SIZE, 0, 0);
        msg_len = sizeof(msg) + msg.part_size;

        if (msg_len <= 0) {
            syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
            exit(EXIT_FAILURE);
        }

        syslog(LOG_INFO, "Msg len: %d", msg_len);

        if (handle_msg(server_buf, &msg)) {
            syslog(LOG_ERR, "msgrcv() - Message handling failed!");
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
