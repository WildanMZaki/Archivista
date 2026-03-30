#ifndef ARCHIVISTA_MOUSE_H
#define ARCHIVISTA_MOUSE_H

#include <windows.h>

LRESULT Mouse_OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam);

#endif //ARCHIVISTA_MOUSE_H