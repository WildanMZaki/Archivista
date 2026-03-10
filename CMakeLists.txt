cmake_minimum_required(VERSION 4.1)
project(Archivista C)

set(CMAKE_C_STANDARD 99)

add_executable(Archivista WIN32
        Source/main.c
        Source/miqdar.c
        Source/menu.c
        Header/main.h
        Header/miqdar.h
        Header/menu.h
)