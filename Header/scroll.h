#ifndef ARCHIVISTA_SCROLL_H
#define ARCHIVISTA_SCROLL_H

#include <windows.h>
#include "../Header/app.h"

void Scroll_EnsureCursorVisible(HWND hWnd);
void Scroll_Vertical(AppState *s, WPARAM wParam);

#endif // ARCHIVISTA_SCROLL_H