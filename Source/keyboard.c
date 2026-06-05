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

static void Keyboard_HandleSelectionDelete(AppState *s, HistoryAction *action)
{
    char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
    if (sel)
    {
        TextPos selectionStart;
        TextPos selectionEnd;

        Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
        HistoryAction_AddEdit(action, HISTORY_EDIT_DELETE, sel, selectionStart.row, selectionStart.col, true);
        free(sel);
        Buffer_DeleteSelection(&s->textBuffer, &s->selection);
        Keyboard_ClearSelection(s);
    }
}

LRESULT Keyboard_OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    char c = (char)wParam;
    HistoryAction action;
    HistoryAction_Init(&action);
    TextPos insertStart = Keyboard_GetInsertStart(&s->textBuffer, &s->selection);

    if (c == '\r')
    {
        Keyboard_HandleSelectionDelete(s, &action);

        Buffer_InsertNewline(&s->textBuffer);
        HistoryAction_AddEdit(&action, HISTORY_EDIT_INSERT, "\n", insertStart.row, insertStart.col, false);
    }
    else if (c == '\b' || c == 127)
    {
        if (Buffer_HasSelection(&s->textBuffer, &s->selection))
        {
            Keyboard_HandleSelectionDelete(s, &action);
        }
        else
        {
            int row = s->textBuffer.cursorRow;
            int col = s->textBuffer.cursorCol;

            if (c == 127)
            {
                if (col > 0)
                {
                    const char *lineText = Buffer_GetLineText(&s->textBuffer, row);
                    int startCol = col;
                    while (startCol > 0 && lineText[startCol - 1] == ' ')
                        startCol--;
                    while (startCol > 0 && lineText[startCol - 1] != ' ')
                        startCol--;
                    
                    int charsToDelete = col - startCol;
                    if (charsToDelete > 0)
                    {
                        char *temp = (char*)malloc((size_t)(charsToDelete + 1));
                        if (temp)
                        {
                            memcpy(temp, &lineText[startCol], (size_t)charsToDelete);
                            temp[charsToDelete] = '\0';
                            HistoryAction_AddEdit(&action, HISTORY_EDIT_DELETE, temp, row, startCol, false);
                            free(temp);
                        }
                        
                        for (int i = 0; i < charsToDelete; i++)
                            Buffer_Backspace(&s->textBuffer);
                    }
                }
                else if (row > 0)
                {
                    HistoryAction_AddEdit(&action, HISTORY_EDIT_DELETE, "\n", row - 1, Buffer_GetLineLen(&s->textBuffer, row - 1), false);
                    Buffer_Backspace(&s->textBuffer);
                }
            }
            else
            {
                if (col > 0)
                {
                    char temp[2] = { Buffer_GetLineText(&s->textBuffer, row)[col - 1], '\0' };
                    HistoryAction_AddEdit(&action, HISTORY_EDIT_DELETE, temp, row, col - 1, false);
                }
                else if (row > 0)
                {
                    HistoryAction_AddEdit(&action, HISTORY_EDIT_DELETE, "\n", row - 1, Buffer_GetLineLen(&s->textBuffer, row - 1), false);
                }

                Buffer_Backspace(&s->textBuffer);
            }
        }
    }
    else if (c == '\t')
    {
        Keyboard_HandleSelectionDelete(s, &action);

        for (int i = 0; i < 4; i++)
            Buffer_InsertChar(&s->textBuffer, ' ');
        HistoryAction_AddEdit(&action, HISTORY_EDIT_INSERT, "    ", insertStart.row, insertStart.col, false);
    }
    else if (c == ' ')
    {
        Keyboard_HandleSelectionDelete(s, &action);

        Buffer_InsertChar(&s->textBuffer, c);
        HistoryAction_AddEdit(&action, HISTORY_EDIT_INSERT, " ", insertStart.row, insertStart.col, false);
    }
    else if (c >= 32 && c != 127)
    {
        Keyboard_HandleSelectionDelete(s, &action);

        Buffer_InsertChar(&s->textBuffer, c);
        char temp[2] = { c, '\0' };
        HistoryAction_AddEdit(&action, HISTORY_EDIT_INSERT, temp, insertStart.row, insertStart.col, false);
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

    int currentLen = Buffer_GetLineLen(&s->textBuffer, row);

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
            col = Buffer_GetLineLen(&s->textBuffer, row);
        }
        break;

    case VK_RIGHT:
        if (col < currentLen)
        {
            col++;
        }
        else if (row < Buffer_GetLineCount(&s->textBuffer) - 1)
        {
            row++;
            col = 0;
        }
        break;

    case VK_UP:
        if (row > 0)
        {
            row--;
            int targetLen = Buffer_GetLineLen(&s->textBuffer, row);
            if (col > targetLen)
                col = targetLen;
        }
        break;

    case VK_DOWN:
        if (row < Buffer_GetLineCount(&s->textBuffer) - 1)
        {
            row++;
            int targetLen = Buffer_GetLineLen(&s->textBuffer, row);
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
        int targetLen = Buffer_GetLineLen(&s->textBuffer, row);
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
        if (row >= Buffer_GetLineCount(&s->textBuffer))
            row = Buffer_GetLineCount(&s->textBuffer) - 1;
        int targetLen = Buffer_GetLineLen(&s->textBuffer, row);
        if (col > targetLen)
            col = targetLen;
        break;
    }

    case VK_DELETE:
    {
        History_RecordAndExecuteDelete(&s->history, &s->textBuffer, &s->selection);
        App_SyncEditedState(s);
        App_RefreshEditorAfterAction(hWnd, s);
        return 0;
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