#include <stdio.h>
#include "common.h"

void get_uid(char *uid) {
    FILE *random_generator = fopen("/dev/urandom", "r");
    fread(uid, sizeof(char), 16, random_generator);
    fclose(random_generator);
}