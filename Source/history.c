#include "../Header/history.h"
#include "../Header/cursor.h"
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

void History_RecordAndExecuteDelete(EditHistory *history, TextBuffer *buf, TextSelection *sel)
{
    if (!history || !buf || !sel) return;

    /* If selection exists, capture it and push as single delete action */
    char *selected = Buffer_GetSelectedString(buf, sel);
    if (selected)
    {
        TextPos selectionStart;
        TextPos selectionEnd;

        Buffer_NormalizeSelection(buf, sel, &selectionStart, &selectionEnd);
        HistoryAction del = History_CreateDeleteAction(selected, selectionStart.row, selectionStart.col);
        History_PushAction(history, del);
        free(selected);
        Buffer_DeleteSelection(buf, sel);
        sel->active = 0;
        return;
    }

    int row = buf->cursorRow;
    int col = buf->cursorCol;
    char deletedText[2] = {0};

    /* Capture what will be deleted */
    if (col < Buffer_GetLineLen(buf, row))
    {
        /* Delete character at cursor */
        deletedText[0] = Buffer_GetLineText(buf, row)[col];
        deletedText[1] = '\0';
    }
    else if (row < Buffer_GetLineCount(buf) - 1)
    {
        /* Delete newline (join with next line) */
        deletedText[0] = '\n';
        deletedText[1] = '\0';
    }

    if (deletedText[0] != '\0') /* Only record if something was actually deleted */
    {
        HistoryAction deleteAction = History_CreateDeleteAction(deletedText, row, col);
        Buffer_Delete(buf);
        History_PushAction(history, deleteAction);
    }
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
bool History_Undo(EditHistory *history, TextBuffer *buffer)
{
    HistoryAction *currentAction;
    void *popData;
    int popSize;

    if (!history || !buffer)
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
    
    /* Reverse edits in reverse order */
    for (int i = currentAction->editCount - 1; i >= 0; i--)
    {
        HistoryEdit *edit = &currentAction->edits[i];
        if (edit->type == HISTORY_EDIT_INSERT)
        {
            Cursor_SetPosition(buffer, edit->row, edit->col);
            for (int j = 0; j < (int)strlen(edit->text); j++)
                Buffer_Delete(buffer);
        }
        else if (edit->type == HISTORY_EDIT_DELETE)
        {
            Cursor_SetPosition(buffer, edit->row, edit->col);
            for (int j = 0; edit->text[j]; j++)
            {
                if (edit->text[j] == '\n')
                    Buffer_InsertNewline(buffer);
                else
                    Buffer_InsertChar(buffer, edit->text[j]);
            }
        }
    }

    /* Push exactly the same action to redo stack */
    Push(&history->redoStack, currentAction, sizeof(HistoryAction));

    /* Since Push copied currentAction's pointers, the redo stack now owns the strings.
       So we only free the container popData, NOT the strings in currentAction! */
    free(popData);
    return true;
}

/* Perform redo */
bool History_Redo(EditHistory *history, TextBuffer *buffer)
{
    HistoryAction *redoAction;
    void *popData;
    int popSize;

    if (!history || !buffer)
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
    
    /* Apply edits in forward order */
    for (int i = 0; i < redoAction->editCount; i++)
    {
        HistoryEdit *edit = &redoAction->edits[i];
        if (edit->type == HISTORY_EDIT_INSERT)
        {
            Cursor_SetPosition(buffer, edit->row, edit->col);
            for (int j = 0; edit->text[j]; j++)
            {
                if (edit->text[j] == '\n')
                    Buffer_InsertNewline(buffer);
                else
                    Buffer_InsertChar(buffer, edit->text[j]);
            }
        }
        else if (edit->type == HISTORY_EDIT_DELETE)
        {
            Cursor_SetPosition(buffer, edit->row, edit->col);
            for (int j = 0; j < (int)strlen(edit->text); j++)
                Buffer_Delete(buffer);
        }
    }

    /* Push exactly the same action to undo stack */
    Push(&history->undoStack, redoAction, sizeof(HistoryAction));

    /* Since Push copied redoAction's pointers, the undo stack now owns the strings.
       So we only free the container popData, NOT the strings in redoAction! */
    free(popData);
    return true;
}