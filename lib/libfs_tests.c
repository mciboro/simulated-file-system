#include "libfs.h"

int main() {
    libfs_create("sample_name.t220xt", 0777);
    libfs_create("sample_name.t221xt", 0664);
    libfs_create("sample_name.t222xt", 0222);
    libfs_create("sample_name.t223xt", 0664);
    libfs_create("sample_name.t224xt", 0444);
    libfs_create("sample_name.t224xt", 0777);
    return 0;
}