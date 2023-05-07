#include "libfs.h"

int main() {
    libfs_create("sample_name.t220xt", 777);
    libfs_create("sample_name.t221xt", 777);
    libfs_create("sample_name.t222xt", 777);
    libfs_create("sample_name.t223xt", 777);
    libfs_create("sample_name.t224xt", 777);

    return 0;
}