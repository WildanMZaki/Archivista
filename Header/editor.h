#ifndef ARCHIVISTA_EDITOR_H
#define ARCHIVISTA_EDITOR_H
#include <windows.h>

#define MAX_LINES 10240
#define MAX_COLS 10240

typedef struct
{
    char text[MAX_LINES][MAX_COLS];
    int lineCount;
    int cursorX;
    int cursorY;
} Editor;

extern Editor editor;
void HandleKeyboardTextInput(HWND hWnd, WPARAM wParam);
void DrawCanvas(HWND hWnd);
BOOL HandleKeyboardforCursor(WPARAM wParam);
void InitializeCanvas(HWND hWnd);
void GetFontSize(HWND hWnd);

#endif //ARCHIVISTA_EDITOR_H