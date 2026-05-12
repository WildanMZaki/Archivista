#include "../Header/keyboard.h"
#include "../Header/app.h"
#include "../Header/cursor.h"
#include "../Header/scroll.h"
#include "../Header/shortcuts.h"
#include <windows.h>

static void Keyboard_ClearSelection(AppState *s)
{
    s->selection.active = 0;
}

static TextPos Keyboard_GetInsertStart(const TextBuffer *buffer, const TextSelection *selection)
{
    TextPos start = {buffer->cursorRow, buffer->cursorCol};

    if (Buffer_HasSelection(buffer, selection))
    {
        TextPos selectionStart;
        TextPos selectionEnd;
        Buffer_NormalizeSelection(buffer, selection, &selectionStart, &selectionEnd);
        start = selectionStart;
    }

    return start;
}

LRESULT Keyboard_OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    char c = (char)wParam;
    HistoryAction action;
    TextPos insertStart = Keyboard_GetInsertStart(&s->textBuffer, &s->selection);

    /* Initialize action */
    action.add.active = false;
    action.add.text[0] = '\0';
    action.add.row = insertStart.row;
    action.add.col = insertStart.col;

    action.delete.active = false;
    action.delete.text[0] = '\0';
    action.delete.row = insertStart.row;
    action.delete.col = insertStart.col;

    if (c == '\r')
    {
        /* If selection exists, record it as a delete action first */
        char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
        if (sel)
        {
            TextPos selectionStart;
            TextPos selectionEnd;

            Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
            action.delete.active = true;
            strcpy_s(action.delete.text, HISTORY_ACTION_BUFFER_SIZE, sel);
            action.delete.row = selectionStart.row;
            action.delete.col = selectionStart.col;
            free(sel);
            Buffer_DeleteSelection(&s->textBuffer, &s->selection);
            Keyboard_ClearSelection(s);
        }

        Buffer_InsertNewline(&s->textBuffer);
        action.add.active = true;
        strcpy_s(action.add.text, sizeof(action.add.text), "\n");
    }
    else if (c == '\b')
    {
        /* If selection exists, capture and delete it */
        char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
        if (sel)
        {
            action.delete.active = true;
            TextPos selectionStart;
            TextPos selectionEnd;

            Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
            strcpy_s(action.delete.text, HISTORY_ACTION_BUFFER_SIZE, sel);
            action.delete.row = selectionStart.row;
            action.delete.col = selectionStart.col;
            free(sel);
            Buffer_DeleteSelection(&s->textBuffer, &s->selection);
            Keyboard_ClearSelection(s);
        }
        else
        {
            /* Capture what will be deleted */
            int row = s->textBuffer.cursorRow;
            int col = s->textBuffer.cursorCol;

            action.delete.active = true;
            if (col > 0)
            {
                /* Delete character before cursor */
                action.delete.text[0] = s->textBuffer.lines[row][col - 1];
                action.delete.text[1] = '\0';
                action.delete.row = row;
                action.delete.col = col - 1;
            }
            else if (row > 0)
            {
                /* Delete newline from previous line */
                action.delete.text[0] = '\n';
                action.delete.text[1] = '\0';
                action.delete.row = row - 1;
                action.delete.col = s->textBuffer.lineLen[row - 1];
            }
            else
            {
                action.delete.active = false;
            }

            Buffer_Backspace(&s->textBuffer);
        }
    }
    else if (c == '\t')
    {
        char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
        if (sel)
        {
            action.delete.active = true;
            TextPos selectionStart;
            TextPos selectionEnd;

            Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
            strcpy_s(action.delete.text, HISTORY_ACTION_BUFFER_SIZE, sel);
            action.delete.row = selectionStart.row;
            action.delete.col = selectionStart.col;
            free(sel);
            Buffer_DeleteSelection(&s->textBuffer, &s->selection);
            Keyboard_ClearSelection(s);
        }

        for (int i = 0; i < 4; i++)
            Buffer_InsertChar(&s->textBuffer, ' ');
        action.add.active = true;
        strcpy_s(action.add.text, sizeof(action.add.text), "    ");
    }
    else if (c == ' ')
    {
        char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
        if (sel)
        {
            action.delete.active = true;
            TextPos selectionStart;
            TextPos selectionEnd;

            Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
            strcpy_s(action.delete.text, HISTORY_ACTION_BUFFER_SIZE, sel);
            action.delete.row = selectionStart.row;
            action.delete.col = selectionStart.col;
            free(sel);
            Buffer_DeleteSelection(&s->textBuffer, &s->selection);
            Keyboard_ClearSelection(s);
        }

        Buffer_InsertChar(&s->textBuffer, c);
        action.add.active = true;
        action.add.text[0] = ' ';
        action.add.text[1] = '\0';
    }
    else if (c >= 32)
    {
        char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
        if (sel)
        {
            action.delete.active = true;
            TextPos selectionStart;
            TextPos selectionEnd;

            Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
            strcpy_s(action.delete.text, HISTORY_ACTION_BUFFER_SIZE, sel);
            action.delete.row = selectionStart.row;
            action.delete.col = selectionStart.col;
            free(sel);
            Buffer_DeleteSelection(&s->textBuffer, &s->selection);
            Keyboard_ClearSelection(s);
        }

        Buffer_InsertChar(&s->textBuffer, c);
        action.add.active = true;
        action.add.text[0] = c;
        action.add.text[1] = '\0';
    }

    /* Push the resulting action */
    History_PushAction(&s->history, action);
    App_SyncEditedState(s);
    App_RefreshEditorAfterAction(hWnd, s);
    return 0;
}

LRESULT Keyboard_OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    if (Shortcuts_HandleKeyDown(hWnd, wParam, lParam))
        return 0;

    int row = s->textBuffer.cursorRow;
    int col = s->textBuffer.cursorCol;

    int currentLen = s->textBuffer.lineLen[row];

    switch (wParam)
    {
    case VK_LEFT:
        if (col > 0)
        {
            col--;
        }
        else if (row > 0)
        {
            row--;
            col = s->textBuffer.lineLen[row];
        }
        break;

    case VK_RIGHT:
        if (col < currentLen)
        {
            col++;
        }
        else if (row < s->textBuffer.lineCount - 1)
        {
            row++;
            col = 0;
        }
        break;

    case VK_UP:
        if (row > 0)
        {
            row--;
            int targetLen = s->textBuffer.lineLen[row];
            if (col > targetLen)
                col = targetLen;
        }
        break;

    case VK_DOWN:
        if (row < s->textBuffer.lineCount - 1)
        {
            row++;
            int targetLen = s->textBuffer.lineLen[row];
            if (col > targetLen)
                col = targetLen;
        }
        break;

    case VK_HOME:
        col = 0;
        break;

    case VK_END:
        col = currentLen;
        break;

    case VK_PRIOR: // Page Up
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int pageLines = (rc.bottom - rc.top) / (s->charHeight ? s->charHeight : 1);
        row -= pageLines;
        if (row < 0)
            row = 0;
        int targetLen = s->textBuffer.lineLen[row];
        if (col > targetLen)
            col = targetLen;
        break;
    }

    case VK_NEXT: // Page Down
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int pageLines = (rc.bottom - rc.top) / (s->charHeight ? s->charHeight : 1);
        row += pageLines;
        if (row >= s->textBuffer.lineCount)
            row = s->textBuffer.lineCount - 1;
        int targetLen = s->textBuffer.lineLen[row];
        if (col > targetLen)
            col = targetLen;
        break;
    }

    case VK_DELETE:
    {
        int row = s->textBuffer.cursorRow;
        int col = s->textBuffer.cursorCol;

        /* If selection exists, capture it and push as single delete action */
        char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
        if (sel)
        {
            TextPos selectionStart;
            TextPos selectionEnd;

            Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
            HistoryAction del = History_CreateDeleteAction(sel, selectionStart.row, selectionStart.col);
            del.delete.row = selectionStart.row;
            del.delete.col = selectionStart.col;
            History_PushAction(&s->history, del);
            free(sel);
            Buffer_DeleteSelection(&s->textBuffer, &s->selection);
            Keyboard_ClearSelection(s);
            App_SyncEditedState(s);
            break;
        }

        char deletedText[2] = {0};
        bool isNewline = false;

        /* Capture what will be deleted */
        if (col < s->textBuffer.lineLen[row])
        {
            /* Delete character at cursor */
            deletedText[0] = s->textBuffer.lines[row][col];
            deletedText[1] = '\0';
        }
        else if (row < s->textBuffer.lineCount - 1)
        {
            /* Delete newline (join with next line) */
            deletedText[0] = '\n';
            deletedText[1] = '\0';
            isNewline = true;
        }

        HistoryAction deleteAction = History_CreateDeleteAction(deletedText, row, col);

        if (deletedText[0] != '\0') /* Only record if something was actually deleted */
        {
            Buffer_Delete(&s->textBuffer);
            History_PushAction(&s->history, deleteAction);
        }
        App_SyncEditedState(s);
        break;
    }

    default:
        return DefWindowProc(hWnd, WM_KEYDOWN, wParam, lParam);
    }

    Cursor_SetPosition(&s->textBuffer, row, col);

    if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN ||
        wParam == VK_HOME || wParam == VK_END || wParam == VK_PRIOR || wParam == VK_NEXT)
    {
        Keyboard_ClearSelection(s);
    }

    App_RefreshEditorAfterAction(hWnd, s);
    return 0;
}