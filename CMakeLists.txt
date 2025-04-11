cmake_minimum_required(VERSION 2.20)
project(T1_Shell C)

set(CMAKE_C_STANDARD 23)

add_executable(T1_Shell main.c
        term_tools.h)
