cmake_minimum_required(VERSION 3.10)

project(app)

set(CMAKE_C_STANDARD 99)

add_executable(app utils.c main.c)

target_link_libraries(app m)