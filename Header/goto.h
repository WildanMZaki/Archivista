#ifndef ARCHIVISTA_GOTO_H
#define ARCHIVISTA_GOTO_H
#include "app.h"

// Context state for the Goto Line dialog
typedef struct {
    HWND hwndOwner;
    AppState *appState;
    HWND hwndEdit;
    HWND hwndOkBtn;
    WNDPROC fnOldEditProc; // Stores original Edit procedure for subclassing
} GotoDlgContext;

LRESULT CALLBACK GotoDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif //ARCHIVISTA_GOTO_H