#include <stdio.h>
#define CTEST_MAIN

#define CTEST_SEGFAULT
#include "ctest.h"

#include "service/operations.h"
#include "service/req_buffer.h"


//CTEST(suite1, test1) {}

// there are many different ASSERT macro's (see ctest.h)
CTEST(suite1, test2) {
    ASSERT_EQUAL(1,1);
}

CTEST(suite2, test1) {
    ASSERT_STR("foo", "foo");
}

CTEST(suite3, test3) {
    ASSERT_EQUAL(1,2);
}

// req_buffer.c tests

// req_buffer_create 
CTEST(suite4, test4) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_buffer_create(&rbuf, 256);
    ASSERT_EQUAL(0, res);
}

CTEST(suite5, test5) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_buffer_create(&rbuf, 0);
    ASSERT_EQUAL(-1, res);
}

// req_buffer_destroy
CTEST(suite6, test6) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_buffer_create(&rbuf, 256);
    int res2 = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(0, res2);
}

CTEST(suite7, test7) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(0, res);
}

CTEST(suite8, test8) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_bufer_destroy(&rbuf);
    int res2 = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(-1, res2);
}

CTEST(suite9, test9) {
    struct req_buffer_t *rbuf = NULL;
    int res = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(-1, res);
}

// handle_msg 
CTEST(suite10, test10) {
    struct req_buffer_t *rbuf = NULL;
    struct request_t *msg = malloc(MAX_MSG_SIZE);
    int res = handle_msg(rbuf, msg);
    ASSERT_EQUAL(-1, res);
}

// get_service_req
CTEST(suite11, test11) {
    struct req_buffer_t *rbuf = NULL;
    struct service_req_t *req = NULL;
    int res = get_service_req(rbuf, &req);
    ASSERT_EQUAL(-1, res);
}

CTEST(suite12, test12) {
    struct req_buffer_t *rbuf = NULL;
    struct service_req_t *req = {0}; 
    int res = get_service_req(rbuf, &req);
    ASSERT_EQUAL(-1, res);
}

//operations.c

//test service_chmod - FIX (sigsegv)
/*CTEST(suite8, test8) {
    //struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    struct service_req_t *req = NULL;
    int res = service_create(req);
    int res2 = service_chmod(req);
    ASSERT_EQUAL(0, res);
}*/

/*CTEST(suite10, test10) {
    //struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    fd_type lib = libfs_create("sample_name.t220xt", 0777);
    
    //ASSERT_EQUAL(fd_type*, typeid(lib));
}*/

int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    printf("\nTest done!\n");
    return result;
}

