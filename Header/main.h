#ifndef PROYEK_2_REAL_MAIN_H
#define PROYEK_2_REAL_MAIN_H
#include <windows.h>
#include "miqdar.h"

#define FONT_NAME "Consolas"
#define FONT_WIDTH 0
#define FONT_HEIGHT 18

extern HWND hEdit;
extern HFONT EditorFont;
LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);

#endif //PROYEK_2_REAL_MAIN_H