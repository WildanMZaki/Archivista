cmake_minimum_required(VERSION 4.1)
project(Archivista VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

configure_file(
        ${CMAKE_SOURCE_DIR}/app.rc.in
        ${CMAKE_BINARY_DIR}/app.rc
        @ONLY
)

add_executable(Archivista WIN32
        Source/app.c
        Source/buffer.c
        Source/cursor.c
        Source/fileops.c
        Source/history.c
        Source/keyboard.c
        Source/main.c
        Source/menu.c
        Source/mouse.c
        Source/recent.c
        Source/render.c
        Source/scroll.c
        Source/selection.c
        Source/shortcuts.c
        Source/window.c
        Source/winproc.c
        Source/stack/stack.c
        Source/clipboard.c
        Source/search.c
        Source/goto.c
        Source/zoom.c
        Source/config.c
        Source/wordwrap.c

        Header/app.h
        Header/buffer.h
        Header/cursor.h
        Header/fileops.h
        Header/history.h
        Header/keyboard.h
        Header/main.h
        Header/menu.h
        Header/mouse.h
        Header/recent.h
        Header/render.h
        Header/scroll.h
        Header/selection.h
        Header/shortcuts.h
        Header/window.h
        Header/winproc.h
        Header/clipboard.h
        Header/search.h
        Header/goto.h
        Header/zoom.h
        Header/utils.h
        Header/config.h
        Header/resource.h
        Header/wordwrap.h

        ${CMAKE_BINARY_DIR}/app.rc
)