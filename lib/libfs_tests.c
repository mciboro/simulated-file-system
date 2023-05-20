#include "libfs.h"

int main() {
    libfs_create("sample_name.t220xt", 0444);
    libfs_create("sample_name.t221xt", 0222);
    libfs_create("sample_name.t222xt", 0667);
    libfs_create("sample_name.t223xt", 0777);
    libfs_create("sample_name.t224xt", 0101);

    libfs_rename("sample_name.t221xt", "new_nice_name.exehehe");
    return 0;
}