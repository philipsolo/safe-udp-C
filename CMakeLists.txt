cmake_minimum_required(VERSION 3.19)
project(csc2035-assignment2-init C)

set(CMAKE_C_STANDARD 99)
add_library(csc2035-assignment2-init rft_client_util.c rft_client_util.h)
add_executable(client ${PROJECT_SOURCE_DIR}/rft_client.c)
add_executable(server rft_server.c)

target_link_libraries(client ${PROJECT_SOURCE_DIR}/rft_client_util.c ${PROJECT_SOURCE_DIR}/rft_util.c ${PROJECT_SOURCE_DIR}/rft_util.h ${PROJECT_SOURCE_DIR}/rft_client_util.h)