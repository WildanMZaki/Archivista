cmake_minimum_required(VERSION 4.1)
project(Archivista C)

set(CMAKE_C_STANDARD 99)

add_executable(Archivista WIN32
        Source/main.c
        Source/window.c
        Source/menu.c
        Header/main.h
        Header/window.h
        Header/menu.h
        Header/editor.h
        Source/editor.c
        Source/mouse.c
        Header/mouse.h
        Header/mouse.h
)