#pragma once

#include <fcntl.h>
#include <memory.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "inode.h"
#include "req_buffer.h"

int service_create(struct service_req_t *req);
int service_rename(struct service_req_t *req);