#include <stdio.h>
#define CTEST_MAIN

#define CTEST_SEGFAULT
#include "ctest.h"

#include "service/operations.h"
#include "service/req_buffer.h"
#include "lib/libfs.h"


// req_buffer.c tests
// req_buffer_create
CTEST(suite1, test1) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_buffer_create(&rbuf, 256);
    ASSERT_EQUAL(0, res);
}

CTEST(suite2, test2) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_buffer_create(&rbuf, 0);
    ASSERT_EQUAL(-1, res);
}

// req_buffer_destroy
CTEST(suite3, test3) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_buffer_create(&rbuf, 256);
    int res2 = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(0, res2);
}

CTEST(suite4, test4) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(0, res);
}

CTEST(suite5, test5) {
    struct req_buffer_t *rbuf = malloc(sizeof(struct req_buffer_t) + sizeof(struct service_req_t) * 256);
    int res = req_bufer_destroy(&rbuf);
    int res2 = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(-1, res2);
}

CTEST(suite6, test6) {
    struct req_buffer_t *rbuf = NULL;
    int res = req_bufer_destroy(&rbuf);
    ASSERT_EQUAL(-1, res);
}

// handle_msg
CTEST(suite7, test7) {
    struct req_buffer_t *rbuf = NULL;
    struct request_t *msg = malloc(MAX_MSG_SIZE);
    int res = handle_msg(rbuf, msg);
    ASSERT_EQUAL(-1, res);
}

// get_service_req
CTEST(suite8, test8) {
    struct req_buffer_t *rbuf = NULL;
    struct service_req_t *req = NULL;
    int res = get_service_req(rbuf, &req);
    ASSERT_EQUAL(-1, res);
}

CTEST(suite9, test9) {
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


// rename should success
CTEST(suite10, test10) {
    fd_type lib = libfs_create("sample13.t228xt", 0777);
    int res = libfs_rename("sample13.t228xt", "new_name_test13.exehe"); 
    ASSERT_EQUAL(res, 0);
}

// rename should fail
CTEST(suite11, test11) {
    fd_type lib = libfs_create("sample_name.t268xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two.t268xt", 0777);
    int res = libfs_rename("sample_name.t268xt", "sample_name_two.t268xt");

    ASSERT_EQUAL(res, -2);
}

//rename should fail
CTEST(suite12, test12) {
    fd_type lib = libfs_create("sample_name15.t228xt", 0777);
    int res = libfs_rename("sample_name15.t228xt", "sample_name15.t228xt");

    ASSERT_NOT_EQUAL(res, 0);
    ASSERT_EQUAL(res, -2);
}

// chmode should success
CTEST(suite13, test13) {
    fd_type lib = libfs_create("sample_name.t228xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two.t228xt", 0664);
    int res = libfs_chmode("sample_name.t228xt", 0664);
    int res2 = libfs_chmode("sample_name_two.t228xt", 0444);

    ASSERT_EQUAL(res, 0);
    ASSERT_EQUAL(res2, 0);
}

// unlink should success
CTEST(suite14, test14) {
    fd_type lib = libfs_create("sample_name14.t228xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two14.t228xt", 0664);
    int res = libfs_unlink("sample_name14.t228xt");

    ASSERT_EQUAL(res, 0);
}

// unlink should fail
CTEST(suite15, test15) { 
    fd_type lib = libfs_create("sample_name15.t228xt", 0777);
    fd_type lib2 = libfs_create("sample_name_two15.t228xt", 0664);
    int res = libfs_unlink("sample_name15.t226xt");

    ASSERT_EQUAL(res, -3);
}

// stat should success
CTEST(suite16, test16) {
    struct stat_t buf9;

    fd_type lib = libfs_create("sample_name16.t328xt", 0777);
    fd_type fd3 = libfs_open("sample_name16.t328xt", READ_ONLY);
    int res = libfs_stat("sample_name16.t328xt", &buf9);

    ASSERT_EQUAL(res, 0);

}

// open should fail
CTEST(suite17, test17) {
    struct stat_t buf;
    char *buf2 = malloc(6000);
    fd_type lib = libfs_create("sample_name21.t428xt", 0777);
    fd_type fd3 = libfs_open("sample_name21.txt", READ_ONLY);

    ASSERT_NOT_EQUAL(fd3, 0);
}

// close should fail
CTEST(suite18, test18) {
    struct stat_t buf;
    char *buf2 = malloc(6000);
    fd_type lib = libfs_create("sample_name.txt", 0777);
    fd_type fd3 = libfs_open("sample_name.txt", WRITE_ONLY);
    int res = libfs_stat("sample_name.txt", &buf);
    int res3 = libfs_close(lib);
    
    ASSERT_EQUAL(res3, -1);
    ASSERT_NOT_EQUAL(res3, 0);
}

// unlink should success, close should fail 
CTEST(suite19, test19) {
    struct stat_t buf;
    char *buf2 = malloc(1000);
    fd_type lib = libfs_create("sample_name3.txt", 0777);
    fd_type fd3 = libfs_open("sample_name3.txt", WRITE_ONLY);
    int res2 = libfs_unlink("sample_name3.txt");
    int res3 = libfs_close(lib);

    ASSERT_EQUAL(res2, 0);
    ASSERT_EQUAL(res3, -1);
}

// write and close should success
CTEST(suite20, test20) {
    libfs_create("sample_24.t225xt", 0777);

    fd_type fd1 = libfs_open("sample_24.t225xt", WRITE_ONLY);
    char *buf24 = malloc(3000);
    memset(buf24, 'a', 3000);
    int res = libfs_write(fd1, buf24, 3000);
    int res2 = libfs_close(fd1);

    ASSERT_EQUAL(res, 0);
    ASSERT_EQUAL(res2, 0);

}

// write should fail, close should success
CTEST(suite21, test21) { 
    libfs_create("sample_10.t225xt", 0777);

    fd_type fd1 = libfs_open("sample_10.t225xt", READ_ONLY);
    char *buf1 = malloc(3000);
    memcpy(buf1, "Hello, world!", 13);
    int res = libfs_write(fd1, buf1, 3000);
    int res2 = libfs_close(fd1);

    ASSERT_EQUAL(res, -1);
    ASSERT_EQUAL(res2, 0);
}

// read should fail - FIX
// CTEST(suite22, test22) {
//     struct stat_t buf;
//     libfs_create("sample_name5.t225xt", &buf);
//     fd_type fd1 = libfs_open("sample_name5.t225xt", WRITE_ONLY);
//     char *buf5 = malloc(3000);
//     memcpy(buf5, "Hello, world!", 13);
//     libfs_write(fd1, buf5, 3000);
//     int res3 = libfs_read(fd1, buf5, 500);
//     libfs_close(fd1);
//     ASSERT_EQUAL(res3, -1);
// }

// seek and read should success
CTEST(suite23, test23) {
    libfs_create("sample_name6.t225xt", 0777);
    char *buf3 = malloc(20);
    fd_type fd4 = libfs_open("sample_name6.t225xt", WRITE_ONLY);
    memcpy(buf3, "Hello, world!", 13);
    libfs_write(fd4, buf3, 14);
    int res = libfs_seek(fd4, 3);
    int res2 = libfs_read(fd4, buf3, 5);
    libfs_close(fd4);

    ASSERT_EQUAL(res, 0);
    ASSERT_EQUAL(res2, 0);

}

// seek and write should fail, read should success
CTEST(suite24, test24) {
    struct stat_t buf;
    libfs_create("sample_name7.t225xt", 0777);
    char *buf4 = malloc(20);
    fd_type fd5 = libfs_open("sample_name7.t225xt", READ_ONLY);
    memcpy(buf4, "Hello, world!", 13);
    int res = libfs_write(fd5, buf4, 14);
    int res2 = libfs_seek(fd5, 3);
    int res3 = libfs_read(fd5, buf4, 5);
    libfs_close(fd5);

    ASSERT_EQUAL(res, -1);
    ASSERT_EQUAL(res2, -1);
    ASSERT_EQUAL(res3, 0);

}

// link should success
CTEST(suite25, test25) {
    libfs_create("sample8.t225xt", 0777);
    int res = libfs_link("sample8.t225xt", "target2.t225xt");
    int res2 = libfs_unlink("target2.t225xt");

    ASSERT_EQUAL(res, 0);
}

// link should fail
CTEST(suite26, test26) {
    struct stat_t buf;
    libfs_create("sample_name9.t225xt", 0777);
    libfs_create("sample_name9.t226xt", 0777);
    int res = libfs_link("sample_name9.t225xt", "sample_name9.t226xt");
    
    ASSERT_EQUAL(res, -2);
}

// symlink should success
CTEST(suite27, test27) {
    struct stat_t buf;
    libfs_create("sample_name27.txt", 0777);
    int res = libfs_symlink("sample_name27.txt", "sample_name_test27.t226xt", 0777);

    ASSERT_NOT_EQUAL(res, -2);
    ASSERT_NOT_EQUAL(res, -3);

}

// symlink should fail
CTEST(suite28, test28) {
    struct stat_t buf;
    libfs_create("sample_name28.txt", 0777);
    int res = libfs_symlink("sample_name28.txt", "sample_name_test27.t226xt", 0777);

    ASSERT_EQUAL(res, -2);
}

// symlink should fail
CTEST(suite29, test29) {
    struct stat_t buf;
    libfs_create("sample_name29.txt", 0777);
    int res = libfs_symlink("sample_29.txt", "sample_name_test29.t226xt", 0777);

    ASSERT_EQUAL(res, -1);
}


int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    printf("\nTest done!\n");
    return result;
}