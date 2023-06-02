#include <stdio.h>
#define CTEST_MAIN

#define CTEST_SEGFAULT
#include "ctest.h"

#include "../service/operations.h"
#include "../service/req_buffer.h"


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

CTEST(suite13, test13) {
    fd_type lib = libfs_create("sample_name.t228xt", 0777);
    int res = libfs_rename("sample_name.t228xt", "new_name_hehe.exehe");  
    ASSERT_EQUAL(res, -2);
}

CTEST(suite14, test14) {
    fd_type lib = libfs_create("sample_name.t268xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two.t268xt", 0777);
    int res = libfs_rename("sample_name.t268xt", "sample_name_two.t268xt");
    ASSERT_EQUAL(res, -2);
}

CTEST(suite15, test15) {
    fd_type lib = libfs_create("sample_name.t228xt", 0777);
    int res = libfs_rename("sample_name.t228xt", "sample_name.t228xt");
    ASSERT_NOT_EQUAL(res, 0);
    ASSERT_EQUAL(res, -2);
}

CTEST(suite16, test16) {
    fd_type lib = libfs_create("sample_name.t228xttt", 0777);
    int res = libfs_rename("sample_name.t228xttt", "new_name.exehe"); 
    ASSERT_NOT_EQUAL(res, 0);
}

CTEST(suite17, test17) {
    fd_type lib = libfs_create("sample_name.t228xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two.t228xt", 0664);
    int res = libfs_chmode("sample_name.t228xt", 0664);
    int res2 = libfs_chmode("sample_name_two.t228xt", 0444);

    ASSERT_EQUAL(res, 0);
    ASSERT_EQUAL(res2, 0);
}

CTEST(suite18, test18) {
    fd_type lib = libfs_create("sample_name.t228xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two.t228xt", 0664);
    int res = libfs_unlink("sample_name.t228xt");
    ASSERT_EQUAL(res, 0);
}

CTEST(suite19, test19) { // 
    fd_type lib = libfs_create("sample_name.t228xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two.t228xt", 0664);
    int res = libfs_unlink("sample_name.t226xt");
    ASSERT_EQUAL(res, -3);
}

CTEST(suite20, test20) { //
    struct stat_t buf9;

    fd_type lib = libfs_create("sample_name.t328xt", 0777);
    fd_type fd3 = libfs_open("sample_name.t328xt", READ_ONLY);
    int res = libfs_stat("sample_name.t328xt", &buf9);
    ASSERT_EQUAL(res, 0);

}

CTEST(suite21, test21) {
    struct stat_t buf;
    char *buf2 = malloc(6000);

    fd_type lib = libfs_create("sample_name.t428xt", 0777);
    fd_type fd3 = libfs_open("sample_name.txt", READ_ONLY);

    ASSERT_NOT_EQUAL(fd3, 0);
}

CTEST(suite22, test22) {
    struct stat_t buf;
    char *buf2 = malloc(6000);

    fd_type lib = libfs_create("sample_name.txt", 0777);
    fd_type fd3 = libfs_open("sample_name.txt", WRITE_ONLY);
    int res = libfs_stat("sample_name.txt", &buf);
    int res3 = libfs_close(lib);
    ASSERT_NOT_EQUAL(res3, 0);

}

CTEST(suite23, test23) {
    struct stat_t buf;
    char *buf2 = malloc(1000);

    fd_type lib = libfs_create("sample_name3.txt", 0777);
    fd_type fd3 = libfs_open("sample_name3.txt", WRITE_ONLY);
    int res2 = libfs_unlink("sample_name3.txt");
    int res3 = libfs_close(lib);

    ASSERT_EQUAL(res2, 0);
    ASSERT_EQUAL(res3, -1);
}

// CTEST(suite24, test24) { //FIX
//     libfs_create("sample_24.t225xt", 0777);
//     fd_type fd1 = libfs_open("sample_24.t225xt", WRITE_ONLY);
//     char *buf24 = malloc(3000);
//     memset(buf24, 'a', 3000);
//     int res = libfs_write(fd1, buf24, 3000);
//     int res2 = libfs_close(fd1);

//     ASSERT_EQUAL(res, 0);
//     ASSERT_EQUAL(res2, 0);

// }

// CTEST(suite25, test25) { // FIX
//     libfs_create("sample_10.t225xt", 0777);
//     fd_type fd1 = libfs_open("sample_10.t225xt", WRITE_ONLY);
//     char *buf1 = malloc(3000);
//     memcpy(buf1, "Hello, world!", 13);
//     int res = libfs_write(fd1, buf1, 3000);
//     int res2 = libfs_close(fd1);

//     ASSERT_EQUAL(res, 0);
//     ASSERT_EQUAL(res2, 0);
// }

// CTEST(suite26, test26) {
//     struct stat_t buf;
//     libfs_create("sample_name5.t225xt", &buf);
//     libfs_create("sample_name5.t226xt", &buf);
//     fd_type fd1 = libfs_open("sample_name5.t225xt", WRITE_ONLY);
//     char *buf1 = malloc(3000);
//     memcpy(buf1, "Hello, world!", 13);
//     libfs_write(fd1, buf1, 3000);
//     int res3 = libfs_read(fd1, buf1, 500);
//     libfs_close(fd1);
//     ASSERT_EQUAL(res3, -1);
// }

// CTEST(suite27, test27) {
//     libfs_create("sample_name6.t225xt", 0777);
//     char *buf3 = malloc(20);
//     fd_type fd4 = libfs_open("sample_name6.t225xt", WRITE_ONLY);
//     memcpy(buf3, "Hello, world!", 13);
//     libfs_write(fd4, buf3, 14);
//     int res = libfs_seek(fd4, 3);
//     libfs_read(fd4, buf3, 5);
//     libfs_close(fd4);
//     ASSERT_EQUAL(res, -1);
// }

// CTEST(suite28, test28) {
//     struct stat_t buf;
//     libfs_create("sample_name7.t225xt", &buf);
//     char *buf4 = malloc(20);
//     fd_type fd5 = libfs_open("sample_name7.t225xt", READ_ONLY);
//     memcpy(buf4, "Hello, world!", 13);
//     libfs_write(fd5, buf4, 14);
//     int res = libfs_seek(fd5, 3);
//     libfs_read(fd5, buf4, 5);
//     libfs_close(fd5);
//     ASSERT_EQUAL(res, -1);
// }

CTEST(suite29, test29) {
    libfs_create("sample8.t225xt", 0777);
    int res = libfs_link("sample8.t225xt", "target2.t225xt");
    int res2 = libfs_unlink("target2.t225xt");
    ASSERT_EQUAL(res, 0);
}

CTEST(suite30, test30) {
    struct stat_t buf;
    libfs_create("sample_name9.t225xt", 0777);
    libfs_create("sample_name9.t226xt", 0777);
    int res = libfs_link("sample_name9.t225xt", "sample_name9.t226xt");
    ASSERT_EQUAL(res, -2);
}

int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    printf("\nTest done!\n");
    return result;
}
