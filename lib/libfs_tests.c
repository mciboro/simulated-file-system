#include "libfs.h"

int main() {
    libfs_create("sample_name.t220xt", 0777);
    libfs_create("sample_name.t221xt", 0664);
    libfs_create("sample_name.t222xt", 0222);
    libfs_create("sample_name.t223xt", 0664);
    libfs_create("sample_name.t224xt", 0444);
    libfs_create("sample_name.t224xt", 0777);

    libfs_chmode("sample_name.t220xt", 0644);
    libfs_chmode("sample_name.t220xt222", 0644);

    libfs_rename("sample_name.t221xt", "new_nice_name.exehehe");
    libfs_rename("sample_name.t223xt", "new_nice_name.exehehe");

    struct stat_t buf;
    libfs_stat("sample_name.t221xt", &buf);
    libfs_stat("sample_name.t222xt", &buf);

    libfs_link("sample_name.t220xt", "sample_name.t225xt");

    // libfs_unlink("sample_name.t221xt");
    // libfs_open("sample_name.t222xt", 0);

    return 0;
}