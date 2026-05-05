#ifndef ARCHIVISTA_MAIN_H
#define ARCHIVISTA_MAIN_H

#include <windows.h>
#include "window.h"

#define FONT_NAME "Consolas"
#define FONT_WIDTH 0
#define FONT_HEIGHT 18

// Padding dari tepi window ke area teks
#define TEXT_PADDING_LEFT 4
#define TEXT_PADDING_TOP 4

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);

#endif // ARCHIVISTA_MAIN_H