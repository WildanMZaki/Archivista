#ifndef ARCHIVISTA_KEYBOARD_H
#define ARCHIVISTA_KEYBOARD_H

#include <windows.h>

LRESULT Keyboard_OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Keyboard_OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);

#endif // ARCHIVISTA_KEYBOARD_H