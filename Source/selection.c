#include "../Header/selection.h"
#include "../Header/main.h"

void GetTextPositionFromMouse(LPARAM lParam, AppState *s, int *outRow, int *outCol) {
    int mouseX = (int)(short)LOWORD(lParam) + s->scrollX - TEXT_PADDING_LEFT;
    int mouseY = (int)(short)HIWORD(lParam) + s->scrollY - TEXT_PADDING_TOP;
    int row = (s->charHeight ? (mouseY / s->charHeight) : 0);
    if (row < 0) row = 0;
    if (row >= s->textBuffer.lineCount) row = s->textBuffer.lineCount - 1;
    int col = (s->charWidth ? ((mouseX + s->charWidth / 2) / s->charWidth) : 0);
    if (col < 0) col = 0;

    int len = s->textBuffer.lineLen[row];
    if (col > len) col = len;
    *outRow = row;
    *outCol = col;
}