cmake_minimum_required(VERSION 3.0.0)
project(libfs VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(service service/service.c service/req_buffer.c)
target_include_directories(service PUBLIC .)
target_link_libraries(service pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
