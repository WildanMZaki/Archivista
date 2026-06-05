#ifndef ARCHIVISTA_WORDWRAP_H
#define ARCHIVISTA_WORDWRAP_H

#include <windows.h>
#include "main.h"

typedef struct AppState AppState;

static inline int CalcWrapCols(int clientWidth, int charWidth)
{
    if (charWidth <= 0) charWidth = 1;
    int wrapCols = (clientWidth - TEXT_PADDING_LEFT) / charWidth;
    return (wrapCols < 10) ? 10 : wrapCols;
}

void WordWrap_Init(HWND hWnd, AppState *s);
void WordWrap_Toggle(HWND hWnd, AppState *s);

#endif // ARCHIVISTA_WORDWRAP_H
