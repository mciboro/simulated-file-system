cmake_minimum_required(VERSION 3.0.0)
project(libfs VERSION 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(service service/service.c service/req_buffer.c service/operations.c service/inode.c common.c)
target_include_directories(service PUBLIC .)
target_link_libraries(service pthread)

add_library(libfs SHARED lib/libfs.c common.c)
target_include_directories(libfs PUBLIC .)
# target_link_libraries(libfs)

add_executable(libfs-tests lib/libfs_tests.c)
target_include_directories(libfs-tests PUBLIC .)
target_link_libraries(libfs-tests libfs)

add_executable(libfs-presentation lib/presentation.c)
target_include_directories(libfs-presentation PUBLIC .)
target_link_libraries(libfs-presentation libfs)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

enable_testing()

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/lib/libfs.c ${PROJECT_SOURCE_DIR}/common.h)
file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/test-*.c)
add_executable(runUnitTests ${TEST_SOURCES} ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/test/ctest.h service/req_buffer.c service/operations.c service/inode.c common.c)

target_link_libraries(runUnitTests pthread)
target_include_directories(runUnitTests PUBLIC .)
add_test(NAME runUnitTests COMMAND runUnitTests)
