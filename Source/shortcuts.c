#include "../Header/shortcuts.h"
#include "../Header/menu.h"

// Centralized keyboard shortcuts routing for WM_KEYDOWN.
BOOL Shortcuts_HandleKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    int shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    (void)lParam;

    if (!ctrlDown)
        return FALSE;

    switch (wParam)
    {
    case 'N':
        SendMessage(hWnd, WM_COMMAND, ID_FILE_NEW, 0);
        return TRUE;

    case 'O':
        SendMessage(hWnd, WM_COMMAND, ID_FILE_OPEN, 0);
        return TRUE;

    case 'S':
        SendMessage(hWnd, WM_COMMAND, shiftDown ? ID_FILE_SAVEAS : ID_FILE_SAVE, 0);
        return TRUE;

    case 'A':
        SendMessage(hWnd, WM_COMMAND, ID_EDIT_SELECTALL, 0);
        return TRUE;

    case 'X':
        SendMessage(hWnd, WM_COMMAND, ID_EDIT_CUT, 0);
        return TRUE;

    case 'C':
        SendMessage(hWnd, WM_COMMAND, ID_EDIT_COPY, 0);
        return TRUE;

    case 'V':
        SendMessage(hWnd, WM_COMMAND, ID_EDIT_PASTE, 0);
        return TRUE;

    case 'Z':
        SendMessage(hWnd, WM_COMMAND, shiftDown ? ID_EDIT_REDO : ID_EDIT_UNDO, 0);
        return TRUE;

    case 'Y':
        SendMessage(hWnd, WM_COMMAND, ID_EDIT_REDO, 0);
        return TRUE;

    default:
        return FALSE;
    }
}
