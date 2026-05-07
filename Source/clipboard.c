#include "../Header/clipboard.h"


void Clipboard_Copy(HWND hWnd,  char *stringIn) {
    if (!OpenClipboard(hWnd)) return;
    EmptyClipboard();

    int len = strlen(stringIn);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    char *buffer = GlobalLock(hMem);
    memcpy(buffer, stringIn, len + 1);
    GlobalUnlock(hMem);

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

char *Clipboard_Paste(HWND hWnd) {
    char *result = NULL;
    if (!OpenClipboard(hWnd)) return result;

    HGLOBAL hMem = GetClipboardData(CF_TEXT);
    if (hMem) {
        char *buffer = GlobalLock(hMem);
        result = _strdup(buffer);
        GlobalUnlock(hMem);
    }


    CloseClipboard();
    return result;
}