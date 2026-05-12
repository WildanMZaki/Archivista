#include "../Header/history.h"
#include <stdlib.h>
#include <string.h>

/* Create insert action */
HistoryAction History_CreateInsertAction(const char *text, int row, int col)
{
    HistoryAction action;

    /* Initialize add part */
    action.add.row = row;
    action.add.col = col;
    action.add.active = true;

    if (text && strlen(text) < HISTORY_ACTION_BUFFER_SIZE)
    {
        strcpy_s(action.add.text, HISTORY_ACTION_BUFFER_SIZE, text);
    }
    else
    {
        action.add.text[0] = '\0';
    }

    /* Initialize delete part - not active for this action type */
    action.delete.row = row;
    action.delete.col = col;
    action.delete.active = false;
    action.delete.text[0] = '\0';

    return action;
}

/* Create delete action */
HistoryAction History_CreateDeleteAction(const char *text, int row, int col)
{
    HistoryAction action;

    /* Initialize delete part */
    action.delete.row = row;
    action.delete.col = col;
    action.delete.active = true;

    if (text && strlen(text) < HISTORY_ACTION_BUFFER_SIZE)
    {
        strcpy_s(action.delete.text, HISTORY_ACTION_BUFFER_SIZE, text);
    }
    else
    {
        action.delete.text[0] = '\0';
    }

    /* Initialize add part - not active for this action type */
    action.add.row = row;
    action.add.col = col;
    action.add.active = false;
    action.add.text[0] = '\0';

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

    /* Check if there's anything meaningful to push */
    bool hasAdd = action.add.active && action.add.text[0] != '\0';
    bool hasDelete = action.delete.active && action.delete.text[0] != '\0';

    if (!hasAdd && !hasDelete)
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

    /* Create reverse action untuk redo - swap add/delete */
    reverseAction.add = currentAction->delete;
    reverseAction.delete = currentAction->add;

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

    /* Create reverse action untuk undo - swap add/delete */
    reverseAction.add = redoAction->delete;
    reverseAction.delete = redoAction->add;

    /* Push reverse action ke undo stack */
    Push(&history->undoStack, &reverseAction, sizeof(HistoryAction));

    free(popData);
    return true;
}