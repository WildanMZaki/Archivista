#ifndef PROYEK_2_REAL_MIQDAR_H
#define PROYEK_2_REAL_MIQDAR_H
#include <windows.h>

#define APP_TITLE "Archivista"
#define EDIT_CONTROL_ID 1
#define APP_WIDTH 500
#define APP_HEIGHT 400

BOOL InitApplication(HINSTANCE hInstance, WNDPROC winProc);
BOOL InitInstance(HINSTANCE hinstance, int nCmdShow);
HWND CreateCanvas(HWND hWnd);
HFONT CustomFontCanvas(LPCSTR fontName, int fontHeight, int fontWidth);

#endif //PROYEK_2_REAL_MIQDAR_H