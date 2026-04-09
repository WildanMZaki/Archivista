cmake_minimum_required(VERSION 4.1)
project(Archivista C)

set(CMAKE_C_STANDARD 99)

add_executable(Archivista WIN32
        Source/main.c
        Source/winproc.c
        Source/app.c
        Source/render.c
        Source/keyboard.c
        Source/scroll.c
        Source/window.c
        Source/menu.c
        Source/shortcuts.c
        Source/buffer.c
        Source/mouse.c
        Header/main.h
        Header/winproc.h
        Header/app.h
        Header/render.h
        Header/keyboard.h
        Header/scroll.h
        Header/window.h
        Header/menu.h
        Header/shortcuts.h
        Header/buffer.h
        Header/mouse.h
        Header/selection.h
        Source/selection.c
        Source/fileops.c
        Header/fileops.h
)