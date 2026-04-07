#include "../Header/keyboard.h"
#include "../Header/app.h"
#include "../Header/main.h"
#include "../Header/scroll.h"
#include "../Header/shortcuts.h"
#include <windows.h>

static void Keyboard_ResetBlink(HWND hWnd, AppState *s)
{
    s->cursorVisible = TRUE;
    SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
}

static void Keyboard_ClearSelection(AppState *s)
{
    s->selection.active = 0;
}

LRESULT Keyboard_OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    char c = (char)wParam;
    HistorySnapshot before = History_Capture(&s->textBuffer, &s->selection);

    if (c == '\r')
    {
        Buffer_DeleteSelection(&s->textBuffer, &s->selection);
        Keyboard_ClearSelection(s);
        Buffer_InsertNewline(&s->textBuffer);
    }
    else if (c == '\b')
    {
        if (Buffer_DeleteSelection(&s->textBuffer, &s->selection))
        {
            Keyboard_ClearSelection(s);
        }
        else
        {
            Buffer_Backspace(&s->textBuffer);
        }
    }
    else if (c == '\t')
    {
        Buffer_DeleteSelection(&s->textBuffer, &s->selection);
        Keyboard_ClearSelection(s);
        for (int i = 0; i < 4; i++)
            Buffer_InsertChar(&s->textBuffer, ' ');
    }
    else if (c >= 32)
    {
        Buffer_DeleteSelection(&s->textBuffer, &s->selection);
        Keyboard_ClearSelection(s);
        Buffer_InsertChar(&s->textBuffer, c);
    }

    History_RecordChange(&s->history, &before, &s->textBuffer, &s->selection);
    Keyboard_ResetBlink(hWnd, s);
    Scroll_EnsureCursorVisible(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
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
        HistorySnapshot before = History_Capture(&s->textBuffer, &s->selection);
        if (Buffer_DeleteSelection(&s->textBuffer, &s->selection))
        {
            Keyboard_ClearSelection(s);
        }
        else
        {
            Buffer_Delete(&s->textBuffer);
        }
        History_RecordChange(&s->history, &before, &s->textBuffer, &s->selection);
        break;
    }

    default:
        return DefWindowProc(hWnd, WM_KEYDOWN, wParam, lParam);
    }

    s->textBuffer.cursorRow = row;
    s->textBuffer.cursorCol = col;

    if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN ||
        wParam == VK_HOME || wParam == VK_END || wParam == VK_PRIOR || wParam == VK_NEXT)
    {
        Keyboard_ClearSelection(s);
    }

    Keyboard_ResetBlink(hWnd, s);
    Scroll_EnsureCursorVisible(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}