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
    Selection_SetSelection(s, 1, row, 0, row, s->textBuffer.lineLen[row]);
    Cursor_SetPosition(&s->textBuffer, row, s->textBuffer.lineLen[row]);
}
void Selection_SelectWord(AppState *s, int row, int col) {
    int startCol = col;
    int endCol = col;
    int len = s->textBuffer.lineLen[row];
    while (startCol > 0 && isalnum((unsigned char)s->textBuffer.lines[row][startCol - 1])) {
        startCol--;
    }
    while (endCol < len && isalnum((unsigned char)s->textBuffer.lines[row][endCol])) {
        endCol++;
    }
    Selection_SetSelection(s, 1, row, startCol, row, endCol);
    Cursor_SetPosition(&s->textBuffer, row, endCol);
}

void Selection_SelectAll(AppState *s) {
    Selection_SetSelection(s, 1, 0, 0, s->textBuffer.lineCount - 1, s->textBuffer.lineLen[s->selection.end.row]);
    Cursor_SetPosition(&s->textBuffer, s->selection.end.row, s->selection.end.col);
}