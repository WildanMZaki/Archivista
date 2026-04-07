#ifndef ARCHIVISTA_SHORTCUTS_H
#define ARCHIVISTA_SHORTCUTS_H

#include <windows.h>

// Return TRUE if key combination is handled as an application shortcut.
BOOL Shortcuts_HandleKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);

#endif // ARCHIVISTA_SHORTCUTS_H
