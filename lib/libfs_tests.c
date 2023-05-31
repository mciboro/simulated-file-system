#include "libfs.h"

int main() {
    libfs_create("sample_name.t220xt", 0777);
    libfs_create("sample_name.t221xt", 0664);
    libfs_create("sample_name.t222xt", 0222);
    libfs_create("sample_name.t223xt", 0664);
    libfs_create("sample_name.t224xt", 0444);
    libfs_create("sample_name.t224xt", 0777);

    libfs_chmode("sample_name.t220234xt", 0644);
    libfs_rename("sample_name.t221xt", "new_nice_name.exehehe");

    struct stat_t buf;
    libfs_stat("sample_name.t221xt", &buf);
    libfs_stat("sample_name.t222xt", &buf);

    libfs_link("sample_name.t220xt", "sample_name.t225xt");

    libfs_symlink("sample_name.t224xt", "sample_name.t226xt", 0777);
    libfs_stat("sample_name.t225xt", &buf);
    libfs_stat("sample_name.t226xt", &buf);
    fd_type fd1 = libfs_open("sample_name.t225xt", WRITE_ONLY);
    fd_type fd2 = libfs_open("sample_name.t226xt", READ_ONLY);
    char *buf1 = malloc(3000);
    memset(buf1, 'a', 3000);
    buf1[0] = 'b';
    libfs_write(fd1, buf1, 3000);
    libfs_write(fd1, buf1, 3000);
    // libfs_write(fd1, buf1, 5000);
    libfs_close(fd1);
    libfs_close(fd2);
    

    char *buf2 = malloc(6000);
    fd_type fd3 = libfs_open("sample_name.t225xt", WRITE_ONLY);
    libfs_stat("sample_name.t225xt", &buf);
    libfs_read(fd3, buf2, 6000);
    libfs_close(fd3);
    for (int i = 0; i < 6000; ++i) {
        if (buf2[i % 2] != buf1[i]) {
            fprintf(stderr, "Error: '%d' '%c' '%c'\n", i, buf1[i], buf2[i]);
            // break;
        }
        // printf("%c", buf2[i]);
    }
    libfs_stat("sample_name.t225xt", &buf);
    free(buf1);
    free(buf2);
    return 0;
}