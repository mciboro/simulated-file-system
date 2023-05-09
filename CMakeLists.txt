cmake_minimum_required(VERSION 3.0.0)
project(libfs VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(service service/service.c service/req_buffer.c service/operations.c common.c)
target_include_directories(service PUBLIC .)
target_link_libraries(service pthread)

add_library(libfs SHARED lib/libfs.c common.c)
target_include_directories(libfs PUBLIC .)
# target_link_libraries(libfs)

add_executable(libfs-tests lib/libfs_tests.c)
target_include_directories(libfs-tests PUBLIC .)
target_link_libraries(libfs-tests libfs)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
