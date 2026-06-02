#ifndef ARCHIVISTA_HISTORY_H
#define ARCHIVISTA_HISTORY_H

#include <stdbool.h>
#include "buffer.h"
#include "../Source/stack/stack.h"

#define HISTORY_ACTION_BUFFER_SIZE 256

typedef struct
{
    char text[HISTORY_ACTION_BUFFER_SIZE];
    int row;
    int col;
    bool active;
} HistoryActionPart;

typedef struct
{
    HistoryActionPart add;    /* Teks yang ditambahkan */
    HistoryActionPart delete; /* Teks yang dihapus */
} HistoryAction;

typedef struct
{
    Stack undoStack;
    Stack redoStack;
} EditHistory;

/* Create insert action */
HistoryAction History_CreateInsertAction(const char *text, int row, int col);

/* Create delete action */
HistoryAction History_CreateDeleteAction(const char *text, int row, int col);

/* Init history */
void History_Init(EditHistory *history);

/* Free history */
void History_Free(EditHistory *history);

/* Clear history */
void History_Clear(EditHistory *history);

/* Check if can undo */
bool History_CanUndo(const EditHistory *history);

/* Check if can redo */
bool History_CanRedo(const EditHistory *history);

/* Push action ke undo stack, auto-combine jika coordinate nyambung */
void History_PushAction(EditHistory *history, HistoryAction action);

/* Perform undo, apply reverse action ke buffer */
bool History_Undo(EditHistory *history, TextBuffer *buffer, HistoryAction *outAction);

/* Perform redo, apply action ke buffer */
bool History_Redo(EditHistory *history, TextBuffer *buffer, HistoryAction *outAction);

#endif // ARCHIVISTA_HISTORY_H