#ifndef ARCHIVISTA_CLIPBOARD_H
#define ARCHIVISTA_CLIPBOARD_H
#include <windows.h>

void Clipboard_Copy(HWND hWnd,  char *stringIn);
char *Clipboard_Paste(HWND hWnd); // caller must free.

#endif //ARCHIVISTA_CLIPBOARD_H