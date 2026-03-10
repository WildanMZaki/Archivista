#ifndef ARCHIVISTA_MIQDAR_H
#define ARCHIVISTA_MIQDAR_H
#include <windows.h>

#define APP_TITLE "Archivista"
#define APP_WIDTH 500
#define APP_HEIGHT 400

BOOL InitApplication(HINSTANCE hInstance, WNDPROC winProc);
BOOL InitInstance(HINSTANCE hinstance, int nCmdShow);
HFONT CustomFontCanvas(LPCSTR fontName, int fontHeight, int fontWidth);

#endif // ARCHIVISTA_MIQDAR_H