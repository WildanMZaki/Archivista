cmake_minimum_required(VERSION 4.1)
project(Proyek_2_Real C)

set(CMAKE_C_STANDARD 99)

add_executable(Proyek_2_Real WIN32 Source/main.c
        Header/main.h
        Source/miqdar.c
        Header/miqdar.h)
