#include "libfs.h"
#include <stdio.h>

int main() {

    FILE *send_file = fopen("lib/send_file.txt", "r");
    fseek(send_file, 0, SEEK_END);
    long send_file_size = ftell(send_file);
    fclose(send_file);

    char read_buf[send_file_size];
    memset(read_buf, 0, send_file_size);
    fd_type read_fd = 0;
   
    libfs_rename("history_text.txt", "changed_name.txt");

    read_fd = libfs_open("changed_name.txt", READ_ONLY);
    if (read_fd == -1) {
        return 0;
    }
    libfs_read(read_fd, read_buf, send_file_size);

    libfs_close(read_fd);

    FILE *receive_file = fopen("lib/received_file.txt", "w");
    fwrite(read_buf, send_file_size, 1, receive_file);

    libfs_unlink("hardlink_history_text.txt");
    libfs_unlink("symlink_history_text.txt");
    libfs_unlink("corrupted_file.txt");

    fclose(receive_file);
    return 0;
}