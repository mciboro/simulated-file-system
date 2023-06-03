#include "libfs.h"

int main() {
    struct stat_t buf = {0};
    libfs_create("sample_name.t220xt", 0777);
    libfs_create("sample_name.t221xt", 0664);
    libfs_create("sample_name.t222xt", 0664);
    libfs_create("sample_name.t223xt", 0664);

    libfs_unlink("sample_name.t223xt");

    libfs_chmode("sample_name.t220xt", 0644);
    libfs_chmode("sample_name.t220xt222", 0644);

    libfs_rename("sample_name.t221xt", "new_nice_name.exehehe");
    libfs_rename("sample_name.t223xt", "new_nice_name.exehehe");

    libfs_stat("sample_name.t221xt", &buf);
    libfs_stat("sample_name.t222xt", &buf);

    libfs_link("sample_name.t221xt", "sample_name.t225xt");

    fd_type fd1 = libfs_open("sample_name.t225xt", WRITE_ONLY);
    char *buf1 = malloc(3000);
    memset(buf1, 'a', 3000);
    memcpy(buf1, "Hello, world!", 13);
    libfs_write(fd1, buf1, 3000);
    libfs_write(fd1, buf1, 3000);
    libfs_close(fd1);

    char *buf2 = malloc(6000);
    fd_type fd3 = libfs_open("sample_name.t225xt", READ_ONLY);
    libfs_stat("sample_name.t225xt", &buf);
    libfs_read(fd3, buf2, 3000);
    libfs_close(fd3);
    
    unsigned ok = 0;
    for (int i = 0; i < 3000; ++i) {
        if (buf2[i] != buf1[i]) {
            fprintf(stderr, "Error: '%d' '%c' '%c'\n", i, buf1[i], buf2[i]);
            ok = 1;
        }
    }
    if (ok == 0) {
        printf("OK\n");
    }
    free(buf1);
    free(buf2);

    libfs_symlink("sample_name.t225xt", "sample_name.t226xt", 0777);

    char *buf3 = malloc(5);
    fd_type fd4 = libfs_open("sample_name.t226xt", READ_ONLY);
    libfs_seek(fd4, 3);
    libfs_read(fd4, buf3, 5);
    libfs_close(fd4);

    printf("Read data: ");
    for (int i = 0; i < 5; ++i) {
        printf("%c", buf3[i]);
    }
    printf("\n");
    free(buf3);

    // libfs_unlink("sample_name.t221xt");
    // libfs_open("sample_name.t222xt", 0);

    return 0;
}