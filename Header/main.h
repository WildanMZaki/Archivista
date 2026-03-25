#ifndef ARCHIVISTA_MAIN_H
#define ARCHIVISTA_MAIN_H
#include <windows.h>
#include "window.h"

#define FONT_NAME "Consolas"

extern int FONT_WIDTH;
extern int FONT_HEIGHT;
LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);

#endif // ARCHIVISTA_MAIN_H