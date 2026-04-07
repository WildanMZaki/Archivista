#ifndef ARCHIVISTA_HISTORY_H
#define ARCHIVISTA_HISTORY_H

#include <windows.h>
#include "buffer.h"

#define HISTORY_CAPACITY 64

typedef struct
{
    char *text;
    TextSelection selection;
    int cursorRow;
    int cursorCol;
} HistorySnapshot;

typedef struct
{
    HistorySnapshot items[HISTORY_CAPACITY];
    int start;
    int count;
} HistoryStack;

typedef struct
{
    HistoryStack undoStack;
    HistoryStack redoStack;
} EditHistory;

HistorySnapshot History_Capture(const TextBuffer *buffer, const TextSelection *selection);
void HistorySnapshot_Free(HistorySnapshot *snapshot);

void History_Init(EditHistory *history);
void History_Free(EditHistory *history);
void History_Clear(EditHistory *history);
int History_CanUndo(const EditHistory *history);
int History_CanRedo(const EditHistory *history);

void History_PushCurrentState(EditHistory *history, const TextBuffer *buffer, const TextSelection *selection);
int History_RecordChange(EditHistory *history, HistorySnapshot *before, const TextBuffer *afterBuffer, const TextSelection *afterSelection);
int History_Undo(EditHistory *history, TextBuffer *buffer, TextSelection *selection);
int History_Redo(EditHistory *history, TextBuffer *buffer, TextSelection *selection);

#endif // ARCHIVISTA_HISTORY_H