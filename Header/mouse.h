#ifndef ARCHIVISTA_MOUSE_H
#define ARCHIVISTA_MOUSE_H

#include <windows.h>

typedef struct AppState AppState;

LRESULT Mouse_OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnMouseHWheel(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Mouse_OnTimer(HWND hWnd, WPARAM wParam, AppState *s);

#endif //ARCHIVISTA_MOUSE_H