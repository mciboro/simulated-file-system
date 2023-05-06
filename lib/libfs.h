#pragma once

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "common.h"

fd_type libfs_create(char *const name, long mode);
int libfs_chmode(char *const name, long mode);
int libfs_rename(const char *oldname, const char *newname);
int libfs_unlink(char *const name);
fd_type libfs_open(char *const name, const int flags);
int libfs_read(const fd_type fd, const char *buf, const unsigned int size);
int libfs_write(const fd_type fd, const char *buf, const unsigned int size);
int libfs_seek(const fd_type fd, const long int offset);
int libfs_close(const fd_type fd);
int libfs_stat(const char *path, struct stat_t *buf);
int libfs_link(const char *oldpath, const char *newpath);
int libfs_symlink(const char *path1, const char *path2);