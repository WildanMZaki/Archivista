#include "../Header/selection.h"
#include "../Header/cursor.h"

void Selection_SetSelection(AppState *s, int active, int startrow, int startcol, int endrow, int endcol) { // Helper function to make it more clean
    s->selection.active = active;
    s->selection.start.row = startrow;
    s->selection.start.col = startcol;
    s->selection.end.row = endrow;
    s->selection.end.col = endcol;
}

void Selection_SelectPoint(AppState *s, int row, int col) {
    Selection_SetSelection(s, 1, row, col, row, col);
    Cursor_SetPosition(&s->textBuffer, row, col);
}
void Selection_SelectLine(AppState *s, int row) {
    int len = Buffer_GetLineLen(&s->textBuffer, row);
    Selection_SetSelection(s, 1, row, 0, row, len);
    Cursor_SetPosition(&s->textBuffer, row, len);
}
void Selection_SelectWord(AppState *s, int row, int col) {
    int startCol = col;
    int endCol = col;
    int len = Buffer_GetLineLen(&s->textBuffer, row);
    const char *line = Buffer_GetLineText(&s->textBuffer, row);
    while (startCol > 0 && isalnum((unsigned char)line[startCol - 1]))
    {
        startCol--;
    }
    while (endCol < len && isalnum((unsigned char)line[endCol]))
    {
        endCol++;
    }
    Selection_SetSelection(s, 1, row, startCol, row, endCol);
    Cursor_SetPosition(&s->textBuffer, row, endCol);
}

void Selection_SelectAll(AppState *s) {
    int lineCount = Buffer_GetLineCount(&s->textBuffer);
    int lastRow = lineCount > 0 ? lineCount - 1 : 0;
    int lastCol = Buffer_GetLineLen(&s->textBuffer, lastRow);

    Selection_SetSelection(s, 1, 0, 0, lastRow, lastCol);
    Cursor_SetPosition(&s->textBuffer, lastRow, lastCol);
}

void Selection_SetEnd(AppState *s, int row, int col) {
    s->selection.end.row = row;
    s->selection.end.col = col;
}