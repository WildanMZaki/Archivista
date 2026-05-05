#include "../Header/history.h"
#include <stdlib.h>
#include <string.h>

/* Create insert action */
HistoryAction History_CreateInsertAction(const char *text, int row, int col)
{
    HistoryAction action;
    action.type = HISTORY_ACTION_INSERT;
    action.row = row;
    action.col = col;

    if (text && strlen(text) < HISTORY_ACTION_BUFFER_SIZE)
    {
        strcpy_s(action.text, HISTORY_ACTION_BUFFER_SIZE, text);
    }
    else
    {
        action.text[0] = '\0';
    }

    return action;
}

/* Create delete action */
HistoryAction History_CreateDeleteAction(const char *text, int row, int col)
{
    HistoryAction action;
    action.type = HISTORY_ACTION_DELETE;
    action.row = row;
    action.col = col;

    if (text && strlen(text) < HISTORY_ACTION_BUFFER_SIZE)
    {
        strcpy_s(action.text, HISTORY_ACTION_BUFFER_SIZE, text);
    }
    else
    {
        action.text[0] = '\0';
    }

    return action;
}

/* Init history */
void History_Init(EditHistory *history)
{
    if (!history)
        return;

    CreateStack(&history->undoStack);
    CreateStack(&history->redoStack);
}

/* Free history */
void History_Free(EditHistory *history)
{
    if (!history)
        return;

    DeleteAll(&history->undoStack);
    DeleteAll(&history->redoStack);
}

/* Clear history */
void History_Clear(EditHistory *history)
{
    History_Free(history);
    History_Init(history);
}

/* Check if can undo */
bool History_CanUndo(const EditHistory *history)
{
    if (!history)
        return false;

    return !isStackEmpty(history->undoStack);
}

/* Check if can redo */
bool History_CanRedo(const EditHistory *history)
{
    if (!history)
        return false;

    return !isStackEmpty(history->redoStack);
}

/* Push action ke undo stack per event */
void History_PushAction(EditHistory *history, HistoryAction action)
{
    if (!history)
        return;

    if (action.text[0] == '\0' && action.type != HISTORY_ACTION_INSERT)
        return;

    Push(&history->undoStack, &action, sizeof(HistoryAction));

    /* Clear redo stack saat ada action baru */
    DeleteAll(&history->redoStack);
}

/* Perform undo */
bool History_Undo(EditHistory *history, TextBuffer *buffer, HistoryAction *outAction)
{
    HistoryAction *currentAction;
    HistoryAction reverseAction;
    void *popData;
    int popSize;

    if (!history || !buffer || !outAction)
        return false;

    if (!History_CanUndo(history))
        return false;

    /* Pop dari undo stack */
    if (!Pop(&history->undoStack, &popData, &popSize))
        return false;

    if (popSize != sizeof(HistoryAction))
    {
        free(popData);
        return false;
    }

    currentAction = (HistoryAction *)popData;
    *outAction = *currentAction;

    /* Create reverse action untuk redo */
    if (currentAction->type == HISTORY_ACTION_INSERT)
    {
        reverseAction = History_CreateDeleteAction(currentAction->text, currentAction->row, currentAction->col);
    }
    else
    {
        reverseAction = History_CreateInsertAction(currentAction->text, currentAction->row, currentAction->col);
    }

    /* Push reverse action ke redo stack */
    Push(&history->redoStack, &reverseAction, sizeof(HistoryAction));

    free(popData);
    return true;
}

/* Perform redo */
bool History_Redo(EditHistory *history, TextBuffer *buffer, HistoryAction *outAction)
{
    HistoryAction *redoAction;
    HistoryAction reverseAction;
    void *popData;
    int popSize;

    if (!history || !buffer || !outAction)
        return false;

    if (!History_CanRedo(history))
        return false;

    /* Pop dari redo stack */
    if (!Pop(&history->redoStack, &popData, &popSize))
        return false;

    if (popSize != sizeof(HistoryAction))
    {
        free(popData);
        return false;
    }

    redoAction = (HistoryAction *)popData;
    *outAction = *redoAction;

    /* Create reverse action untuk undo */
    if (redoAction->type == HISTORY_ACTION_INSERT)
    {
        reverseAction = History_CreateDeleteAction(redoAction->text, redoAction->row, redoAction->col);
    }
    else
    {
        reverseAction = History_CreateInsertAction(redoAction->text, redoAction->row, redoAction->col);
    }

    /* Push reverse action ke undo stack */
    Push(&history->undoStack, &reverseAction, sizeof(HistoryAction));

    free(popData);
    return true;
}