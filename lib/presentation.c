#include "libfs.h"
#include <stdio.h>

int main() {
    struct stat_t stat = {0};

    FILE *send_file = fopen("lib/send_file.txt", "r");
    fseek(send_file, 0, SEEK_END);
    long send_file_size = ftell(send_file);
    fseek(send_file, 0, SEEK_SET);
    char *send_buffer = (char *)malloc(send_file_size + 1);
    size_t read_size = fread(send_buffer, 1, send_file_size, send_file);
    fclose(send_file);

    char read_buf[send_file_size];
    memset(read_buf, 0, send_file_size);
    fd_type write_fd = 0, read_fd = 0;
    libfs_create("history_text.txt", 0200);
    write_fd = libfs_open("history_text.txt", WRITE_ONLY);
    if (write_fd == -1) {
        return -1;
    }
    libfs_write(write_fd, send_buffer, send_file_size);

    libfs_create("history_text.txt", 0777);

    fd_type second_writer_fd = libfs_open("history_text.txt", WRITE_ONLY);
    if (second_writer_fd == -1) {
        printf("One writer lock works!\n");
    } else {
        libfs_close(second_writer_fd);
    }

    libfs_close(write_fd);

    read_fd = libfs_open("history_text.txt", READ_ONLY);
    if (read_fd == -1) {
        libfs_chmode("history_text.txt", 0660);
    }

    libfs_link("history_text.txt", "hardlink_history_text.txt");
    libfs_symlink("hardlink_history_text.txt", "symlink_history_text.txt", 0400);

    read_fd = libfs_open("symlink_history_text.txt", READ_ONLY);
    if (read_fd == -1) {
        return -1;
    }

    libfs_read(read_fd, read_buf, send_file_size);

    libfs_close(read_fd);

    FILE *receive_file = fopen("lib/received_file.txt", "w");
    fwrite(read_buf, send_file_size, 1, receive_file);

    free(send_buffer);

    libfs_stat("history_text.txt", &stat);
    libfs_stat("symlink_history_text.txt", &stat);

    fclose(receive_file);
    return 0;
}