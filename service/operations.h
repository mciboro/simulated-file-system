#pragma once

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>

#include "common.h"
#include "req_buffer.h"

int service_create(struct service_req_t *req);