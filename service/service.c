#include "common.h"
#include "inode.h"
#include "operations.h"
#include "req_buffer.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define DEBUG

volatile sig_atomic_t working = true;
struct req_buffer_t *server_buf = NULL;
pthread_t receiver = {0}, worker = {0};

void end_service() {
    working = false;
    sem_post(&server_buf->full);
}

void *receive(void *service_args) {
    int msgid = msgget(IPC_REQUESTS_KEY, IPC_PERMS | IPC_CREAT);

    if (msgid == -1) {
        syslog(LOG_ERR, "Error in msgget()");
        exit(EXIT_FAILURE);
    }

    while (working) {
        struct request_t *msg = malloc(MAX_MSG_SIZE);
        memset(msg, 0, MAX_MSG_SIZE);
        int msg_len = 0;

        syslog(LOG_INFO, "Waiting for request...");
        if (msgrcv(msgid, msg, MAX_MSG_SIZE, 0, 0) == -1) {
            if (errno == EINTR) {
                syslog(LOG_INFO, "Interrupted by signal. Breaking...");
                free(msg);
                break;
            }
        }

        syslog(LOG_INFO, "Received request");

        msg_len = sizeof(msg) + msg->part_size;

        if (msg_len <= 0) {
            syslog(LOG_ERR, "Error in msgrcv() - msg size: %d", msg_len);
            exit(EXIT_FAILURE);
        }

        if (handle_msg(server_buf, msg)) {
            syslog(LOG_ERR, "msgrcv() - Message handling failed!");
            exit(EXIT_FAILURE);
        }

        free(msg);
    }

    syslog(LOG_INFO, "Receive thread ending now...");
}

void *operate(void *worker_args) {
    int status = 0;

    for (;;) {
        struct service_req_t *req = NULL;
        status = get_service_req(server_buf, &req);

        if (!working) {
            break;
        }

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
            service_create(req);
            break;
        case CHMODE:
            service_chmode(req);
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
    }

    syslog(LOG_INFO, "Operate thread ending now...");
    pthread_kill(receiver, SIGINT);
}

int main() {
#ifndef DEBUG
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error in fork()");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

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
#endif

    umask(0);

    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        syslog(LOG_ERR, "Cannot retrieve info about service's owner");
        exit(EXIT_FAILURE);
    }

    char *libfs_dir = strcat(pw->pw_dir, "/libfs");

    if (mkdir(libfs_dir, S_IRWXU) == -1 && errno != EEXIST) {
        syslog(LOG_INFO, "Cannot make directory for libfs service");
        exit(EXIT_FAILURE);
    }

    open_inode_table(&inode_table);

    openlog("libfs-service", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Libfs service started");

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

    pthread_create(&receiver, NULL, receive, NULL);
    pthread_create(&worker, NULL, operate, NULL);

    pthread_join(receiver, NULL);
    pthread_join(worker, NULL);

    req_bufer_destroy(&server_buf);
    close_inode_table(inode_table);
    close_file_descriptors_table(descriptor_table);

    syslog(LOG_INFO, "Libfs service ended");
    closelog();
    exit(EXIT_SUCCESS);
}
