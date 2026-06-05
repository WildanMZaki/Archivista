#ifndef ARCHIVISTA_RENDER_H
#define ARCHIVISTA_RENDER_H

#include <windows.h>
#include "main.h"

void Render_CalcCharSize(HWND hWnd);
LRESULT Render_OnPaint(HWND hWnd);

static inline int CalcWrapCols(int clientWidth, int charWidth)
{
    if (charWidth <= 0) charWidth = 1;
    int wrapCols = (clientWidth - TEXT_PADDING_LEFT) / charWidth;
    return (wrapCols < 10) ? 10 : wrapCols;
}

#endif