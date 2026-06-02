#include "../Header/history.h"
#include <stdlib.h>
#include <string.h>

/* Helper functions for History memory management */
void HistoryPart_Init(HistoryActionPart *part)
{
    if (part)
    {
        part->text = NULL;
        part->row = 0;
        part->col = 0;
        part->active = false;
    }
}

void HistoryPart_Free(HistoryActionPart *part)
{
    if (part && part->text)
    {
        free(part->text);
        part->text = NULL;
    }
}

void HistoryPart_SetText(HistoryActionPart *part, const char *newText)
{
    if (!part)
        return;

    HistoryPart_Free(part);

    if (newText)
    {
        size_t len = strlen(newText);
        part->text = (char *)malloc(len + 1);
        if (part->text)
        {
            strcpy_s(part->text, len + 1, newText);
        }
    }
}

void HistoryPart_SetTextNormalized(HistoryActionPart *part, const char *src)
{
    if (!part)
        return;

    HistoryPart_Free(part);

    if (!src)
        return;

    // Calculate normalized length
    size_t len = 0;
    const char *p = src;
    while (*p != '\0')
    {
        if (*p == '\r')
        {
            if (*(p + 1) == '\n')
                p++;
            len++;
        }
        else
        {
            len++;
        }
        p++;
    }

    part->text = (char *)malloc(len + 1);
    if (!part->text)
        return;

    size_t out = 0;
    p = src;
    while (*p != '\0')
    {
        if (*p == '\r')
        {
            if (*(p + 1) == '\n')
                p++;
            part->text[out++] = '\n';
        }
        else
        {
            part->text[out++] = *p;
        }
        p++;
    }
    part->text[out] = '\0';
}

void HistoryPart_Copy(HistoryActionPart *dest, const HistoryActionPart *src)
{
    if (!dest || !src)
        return;

    dest->active = src->active;
    dest->row = src->row;
    dest->col = src->col;
    dest->text = NULL;

    if (src->text)
    {
        size_t len = strlen(src->text);
        dest->text = (char *)malloc(len + 1);
        if (dest->text)
        {
            strcpy_s(dest->text, len + 1, src->text);
        }
    }
}

void HistoryAction_Init(HistoryAction *action)
{
    if (action)
    {
        HistoryPart_Init(&action->add);
        HistoryPart_Init(&action->delete);
    }
}

void HistoryAction_Free(HistoryAction *action)
{
    if (action)
    {
        HistoryPart_Free(&action->add);
        HistoryPart_Free(&action->delete);
    }
}

void HistoryAction_Copy(HistoryAction *dest, const HistoryAction *src)
{
    if (dest && src)
    {
        HistoryPart_Copy(&dest->add, &src->add);
        HistoryPart_Copy(&dest->delete, &src->delete);
    }
}

static void History_FreeStack(Stack *S)
{
    address P;
    while (!isStackEmpty(*S))
    {
        DelLast(S, &P);
        if (P != NULL)
        {
            if (P->data != NULL)
            {
                HistoryAction *action = (HistoryAction *)P->data;
                HistoryAction_Free(action);
            }
            DeAlokasi(P);
        }
    }
}

/* Create insert action */
HistoryAction History_CreateInsertAction(const char *text, int row, int col)
{
    HistoryAction action;
    HistoryAction_Init(&action);

    /* Initialize add part */
    action.add.row = row;
    action.add.col = col;
    action.add.active = true;
    HistoryPart_SetTextNormalized(&action.add, text);

    /* Initialize delete part - not active for this action type */
    action.delete.row = row;
    action.delete.col = col;
    action.delete.active = false;

    return action;
}

/* Create delete action */
HistoryAction History_CreateDeleteAction(const char *text, int row, int col)
{
    HistoryAction action;
    HistoryAction_Init(&action);

    /* Initialize delete part */
    action.delete.row = row;
    action.delete.col = col;
    action.delete.active = true;
    HistoryPart_SetTextNormalized(&action.delete, text);

    /* Initialize add part - not active for this action type */
    action.add.row = row;
    action.add.col = col;
    action.add.active = false;

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

    History_FreeStack(&history->undoStack);
    History_FreeStack(&history->redoStack);
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
    bool hasAdd = action.add.active && action.add.text && action.add.text[0] != '\0';
    bool hasDelete = action.delete.active && action.delete.text && action.delete.text[0] != '\0';

    if (!hasAdd && !hasDelete)
    {
        HistoryAction_Free(&action);
        return;
    }

    Push(&history->undoStack, &action, sizeof(HistoryAction));

    /* Clear redo stack saat ada action baru */
    History_FreeStack(&history->redoStack);
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
    
    /* Deep copy to outAction so the caller gets a copy of strings they own and must free */
    HistoryAction_Copy(outAction, currentAction);

    /* Create reverse action untuk redo - swap add/delete. Deep copy components! */
    HistoryAction_Init(&reverseAction);
    HistoryPart_Copy(&reverseAction.add, &currentAction->delete);
    HistoryPart_Copy(&reverseAction.delete, &currentAction->add);

    /* Push reverse action ke redo stack */
    Push(&history->redoStack, &reverseAction, sizeof(HistoryAction));

    /* Clean up the popped action */
    HistoryAction_Free(currentAction);
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
    
    /* Deep copy to outAction so the caller gets a copy of strings they own and must free */
    HistoryAction_Copy(outAction, redoAction);

    /* Create reverse action untuk undo - swap add/delete. Deep copy components! */
    HistoryAction_Init(&reverseAction);
    HistoryPart_Copy(&reverseAction.add, &redoAction->delete);
    HistoryPart_Copy(&reverseAction.delete, &redoAction->add);

    /* Push reverse action ke undo stack */
    Push(&history->undoStack, &reverseAction, sizeof(HistoryAction));

    /* Clean up the popped action */
    HistoryAction_Free(redoAction);
    free(popData);
    return true;
}