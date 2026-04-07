#include "../Header/history.h"
#include <stdlib.h>
#include <string.h>

static void HistoryStack_Clear(HistoryStack *stack)
{
    if (!stack)
        return;

    for (int i = 0; i < stack->count; i++)
    {
        int index = (stack->start + i) % HISTORY_CAPACITY;
        HistorySnapshot_Free(&stack->items[index]);
    }

    stack->start = 0;
    stack->count = 0;
}

static int HistoryStack_TopIndex(const HistoryStack *stack)
{
    if (!stack || stack->count <= 0)
        return -1;

    return (stack->start + stack->count - 1) % HISTORY_CAPACITY;
}

static int HistoryStack_IsEmpty(const HistoryStack *stack)
{
    return !stack || stack->count <= 0;
}

static int HistorySnapshot_Equals(const HistorySnapshot *a, const HistorySnapshot *b);

static void HistoryStack_Push(HistoryStack *stack, HistorySnapshot snapshot)
{
    if (!stack)
    {
        HistorySnapshot_Free(&snapshot);
        return;
    }

    if (stack->count > 0)
    {
        int topIndex = HistoryStack_TopIndex(stack);
        if (topIndex >= 0 && HistorySnapshot_Equals(&stack->items[topIndex], &snapshot))
        {
            HistorySnapshot_Free(&snapshot);
            return;
        }
    }

    if (stack->count == HISTORY_CAPACITY)
    {
        HistorySnapshot_Free(&stack->items[stack->start]);
        stack->items[stack->start] = snapshot;
        stack->start = (stack->start + 1) % HISTORY_CAPACITY;
        return;
    }

    int index = (stack->start + stack->count) % HISTORY_CAPACITY;
    stack->items[index] = snapshot;
    stack->count++;
}

static int HistoryStack_Pop(HistoryStack *stack, HistorySnapshot *outSnapshot)
{
    if (!stack || !outSnapshot || stack->count <= 0)
        return 0;

    int index = (stack->start + stack->count - 1) % HISTORY_CAPACITY;
    *outSnapshot = stack->items[index];
    stack->items[index].text = NULL;
    stack->count--;
    return 1;
}

static int HistorySnapshot_Equals(const HistorySnapshot *a, const HistorySnapshot *b)
{
    if (!a || !b)
        return 0;

    if (a->cursorRow != b->cursorRow || a->cursorCol != b->cursorCol)
        return 0;

    if (a->selection.active != b->selection.active)
        return 0;

    if (a->selection.start.row != b->selection.start.row ||
        a->selection.start.col != b->selection.start.col ||
        a->selection.end.row != b->selection.end.row ||
        a->selection.end.col != b->selection.end.col)
        return 0;

    if (!a->text || !b->text)
        return a->text == b->text;

    return strcmp(a->text, b->text) == 0;
}

static void History_ApplySnapshot(TextBuffer *buffer, TextSelection *selection, const HistorySnapshot *snapshot)
{
    if (!buffer || !selection || !snapshot)
        return;

    Buffer_FromString(buffer, snapshot->text ? snapshot->text : "");
    buffer->cursorRow = snapshot->cursorRow;
    buffer->cursorCol = snapshot->cursorCol;
    *selection = snapshot->selection;
}

HistorySnapshot History_Capture(const TextBuffer *buffer, const TextSelection *selection)
{
    HistorySnapshot snapshot;
    snapshot.text = NULL;
    snapshot.selection.active = 0;
    snapshot.selection.start.row = 0;
    snapshot.selection.start.col = 0;
    snapshot.selection.end.row = 0;
    snapshot.selection.end.col = 0;
    snapshot.cursorRow = 0;
    snapshot.cursorCol = 0;

    if (buffer)
    {
        snapshot.text = Buffer_ToString((TextBuffer *)buffer);
        snapshot.cursorRow = buffer->cursorRow;
        snapshot.cursorCol = buffer->cursorCol;
    }

    if (selection)
        snapshot.selection = *selection;

    return snapshot;
}

void HistorySnapshot_Free(HistorySnapshot *snapshot)
{
    if (!snapshot)
        return;

    free(snapshot->text);
    snapshot->text = NULL;
}

void History_Init(EditHistory *history)
{
    if (!history)
        return;

    history->undoStack.start = 0;
    history->undoStack.count = 0;
    history->redoStack.start = 0;
    history->redoStack.count = 0;
}

void History_Free(EditHistory *history)
{
    if (!history)
        return;

    HistoryStack_Clear(&history->undoStack);
    HistoryStack_Clear(&history->redoStack);
}

void History_Clear(EditHistory *history)
{
    History_Free(history);
    History_Init(history);
}

int History_CanUndo(const EditHistory *history)
{
    return history && !HistoryStack_IsEmpty(&history->undoStack);
}

int History_CanRedo(const EditHistory *history)
{
    return history && !HistoryStack_IsEmpty(&history->redoStack);
}

void History_PushCurrentState(EditHistory *history, const TextBuffer *buffer, const TextSelection *selection)
{
    if (!history)
        return;

    HistorySnapshot snapshot = History_Capture(buffer, selection);
    if (!snapshot.text)
        return;

    HistoryStack_Push(&history->undoStack, snapshot);
    HistoryStack_Clear(&history->redoStack);
}

int History_RecordChange(EditHistory *history, HistorySnapshot *before, const TextBuffer *afterBuffer, const TextSelection *afterSelection)
{
    HistorySnapshot after;
    int changed;

    if (!history || !before)
    {
        HistorySnapshot_Free(before);
        return 0;
    }

    after = History_Capture(afterBuffer, afterSelection);
    if (!after.text)
    {
        HistorySnapshot_Free(before);
        return 0;
    }

    changed = !HistorySnapshot_Equals(before, &after);
    HistorySnapshot_Free(&after);

    if (!changed)
    {
        HistorySnapshot_Free(before);
        return 0;
    }

    HistoryStack_Push(&history->undoStack, *before);
    before->text = NULL;
    HistoryStack_Clear(&history->redoStack);
    return 1;
}

int History_Undo(EditHistory *history, TextBuffer *buffer, TextSelection *selection)
{
    HistorySnapshot current;
    HistorySnapshot target;

    if (!history || !buffer || !selection || HistoryStack_IsEmpty(&history->undoStack))
        return 0;

    current = History_Capture(buffer, selection);
    if (!current.text)
        return 0;

    if (!HistoryStack_Pop(&history->undoStack, &target))
    {
        HistorySnapshot_Free(&current);
        return 0;
    }

    HistoryStack_Push(&history->redoStack, current);
    History_ApplySnapshot(buffer, selection, &target);
    HistorySnapshot_Free(&target);
    return 1;
}

int History_Redo(EditHistory *history, TextBuffer *buffer, TextSelection *selection)
{
    HistorySnapshot current;
    HistorySnapshot target;

    if (!history || !buffer || !selection || HistoryStack_IsEmpty(&history->redoStack))
        return 0;

    current = History_Capture(buffer, selection);
    if (!current.text)
        return 0;

    if (!HistoryStack_Pop(&history->redoStack, &target))
    {
        HistorySnapshot_Free(&current);
        return 0;
    }

    HistoryStack_Push(&history->undoStack, current);
    History_ApplySnapshot(buffer, selection, &target);
    HistorySnapshot_Free(&target);
    return 1;
}