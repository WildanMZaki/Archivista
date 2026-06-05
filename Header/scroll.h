#ifndef ARCHIVISTA_SCROLL_H
#define ARCHIVISTA_SCROLL_H

#include <windows.h>
#include "../Header/app.h"

void Scroll_EnsureCursorVisible(HWND hWnd);
void Scroll_UpdateScrollbars(HWND hWnd);
void Scroll_Vertical(HWND hWnd, AppState *s, WPARAM wParam);
void Scroll_Horizontal(HWND hWnd, AppState *s, WPARAM wParam);
void Scroll_OnVerticalScroll(HWND hWnd, WPARAM wParam);
void Scroll_OnHorizontalScroll(HWND hWnd, WPARAM wParam);

#endif // ARCHIVISTA_SCROLL_H