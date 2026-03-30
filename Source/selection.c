#include "../Header/selection.h"
#include "../Header/main.h"

void Selection_SelectPoint(AppState *s, int row, int col) {
    s->selection.active = 1;
    s->selection.start.row = row;
    s->selection.start.col = col;
    s->selection.end.row = row;
    s->selection.end.col = col;

    s->textBuffer.cursorRow = row;
    s->textBuffer.cursorCol = col;
}
void Selection_SelectLine(AppState *s, int row) {
    s->selection.active = 1;
    s->selection.start.row = row;
    s->selection.start.col = 0;
    s->selection.end.row = row;
    s->selection.end.col = s->textBuffer.lineLen[row];
    s->textBuffer.cursorRow = row;
    s->textBuffer.cursorCol = s->textBuffer.lineLen[row];
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
    s->selection.active = 1;
    s->selection.start.row = row;
    s->selection.start.col = startCol;
    s->selection.end.row = row;
    s->selection.end.col = endCol;
    s->textBuffer.cursorRow = row;
    s->textBuffer.cursorCol = endCol;
}