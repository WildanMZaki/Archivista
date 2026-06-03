#include "../Header/history.h"
#include <stdlib.h>
#include <string.h>

/* Helper functions for History memory management */
void HistoryAction_Init(HistoryAction *action)
{
    if (action)
    {
        action->edits = NULL;
        action->editCount = 0;
    }
}

void HistoryAction_Free(HistoryAction *action)
{
    if (action)
    {
        if (action->edits)
        {
            for (int i = 0; i < action->editCount; i++)
            {
                if (action->edits[i].text)
                {
                    free(action->edits[i].text);
                }
            }
            free(action->edits);
            action->edits = NULL;
        }
        action->editCount = 0;
    }
}

void HistoryAction_AddEdit(HistoryAction *action, HistoryEditType type, const char *text, int row, int col, bool normalize)
{
    if (!action || !text)
        return;

    HistoryEdit *newEdits = (HistoryEdit *)realloc(action->edits, (action->editCount + 1) * sizeof(HistoryEdit));
    if (!newEdits)
        return;

    action->edits = newEdits;
    HistoryEdit *edit = &action->edits[action->editCount];
    edit->type = type;
    edit->row = row;
    edit->col = col;
    edit->text = NULL;

    if (normalize)
    {
        // Normalization: \r\n or \r to \n
        size_t len = 0;
        const char *p = text;
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

        edit->text = (char *)malloc(len + 1);
        if (edit->text)
        {
            size_t out = 0;
            p = text;
            while (*p != '\0')
            {
                if (*p == '\r')
                {
                    if (*(p + 1) == '\n')
                        p++;
                    edit->text[out++] = '\n';
                }
                else
                {
                    edit->text[out++] = *p;
                }
                p++;
            }
            edit->text[out] = '\0';
        }
    }
    else
    {
        size_t len = strlen(text);
        edit->text = (char *)malloc(len + 1);
        if (edit->text)
        {
            strcpy_s(edit->text, len + 1, text);
        }
    }

    action->editCount++;
}

void HistoryAction_Copy(HistoryAction *dest, const HistoryAction *src)
{
    if (!dest || !src)
        return;

    dest->editCount = 0;
    dest->edits = NULL;

    if (src->editCount > 0)
    {
        dest->edits = (HistoryEdit *)malloc(src->editCount * sizeof(HistoryEdit));
        if (dest->edits)
        {
            dest->editCount = src->editCount;
            for (int i = 0; i < src->editCount; i++)
            {
                dest->edits[i].type = src->edits[i].type;
                dest->edits[i].row = src->edits[i].row;
                dest->edits[i].col = src->edits[i].col;
                dest->edits[i].text = NULL;

                if (src->edits[i].text)
                {
                    size_t len = strlen(src->edits[i].text);
                    dest->edits[i].text = (char *)malloc(len + 1);
                    if (dest->edits[i].text)
                    {
                        strcpy_s(dest->edits[i].text, len + 1, src->edits[i].text);
                    }
                }
            }
        }
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
    HistoryAction_AddEdit(&action, HISTORY_EDIT_INSERT, text, row, col, true);
    return action;
}

/* Create delete action */
HistoryAction History_CreateDeleteAction(const char *text, int row, int col)
{
    HistoryAction action;
    HistoryAction_Init(&action);
    HistoryAction_AddEdit(&action, HISTORY_EDIT_DELETE, text, row, col, true);
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
    if (action.editCount == 0)
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

    /* Push exactly the same action to redo stack (transferring ownership is fine since it's copied via Push's memcpy) */
    Push(&history->redoStack, currentAction, sizeof(HistoryAction));

    /* Since Push copied currentAction's pointers, the redo stack now owns the strings.
       So we only free the container popData, NOT the strings in currentAction! */
    free(popData);
    return true;
}

/* Perform redo */
bool History_Redo(EditHistory *history, TextBuffer *buffer, HistoryAction *outAction)
{
    HistoryAction *redoAction;
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

    /* Push exactly the same action to undo stack (transferring ownership is fine since it's copied via Push's memcpy) */
    Push(&history->undoStack, redoAction, sizeof(HistoryAction));

    /* Since Push copied redoAction's pointers, the undo stack now owns the strings.
       So we only free the container popData, NOT the strings in redoAction! */
    free(popData);
    return true;
}