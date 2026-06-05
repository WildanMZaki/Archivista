#ifndef ARCHIVISTA_HISTORY_H
#define ARCHIVISTA_HISTORY_H

#include <stdbool.h>
#include "buffer.h"
#include "../Source/stack/stack.h"

typedef enum
{
    HISTORY_EDIT_INSERT,
    HISTORY_EDIT_DELETE
} HistoryEditType;

typedef struct
{
    HistoryEditType type;
    char *text;
    int row;
    int col;
} HistoryEdit;

typedef struct
{
    HistoryEdit *edits;
    int editCount;
} HistoryAction;

typedef struct
{
    Stack undoStack;
    Stack redoStack;
} EditHistory;

/* Helper functions for History memory management */
void HistoryAction_Init(HistoryAction *action);
void HistoryAction_Free(HistoryAction *action);
void HistoryAction_AddEdit(HistoryAction *action, HistoryEditType type, const char *text, int row, int col, bool normalize);
void HistoryAction_Copy(HistoryAction *dest, const HistoryAction *src);

/* Create insert action */
HistoryAction History_CreateInsertAction(const char *text, int row, int col);

/* Create delete action */
HistoryAction History_CreateDeleteAction(const char *text, int row, int col);

/* Helper to record and execute delete (either selection or single char) */
void History_RecordAndExecuteDelete(EditHistory *history, TextBuffer *buf, TextSelection *sel);

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
bool History_Undo(EditHistory *history, TextBuffer *buffer);

/* Perform redo, apply action ke buffer */
bool History_Redo(EditHistory *history, TextBuffer *buffer);

#endif // ARCHIVISTA_HISTORY_H