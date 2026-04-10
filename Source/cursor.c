#include "../Header/cursor.h"
#include "../Header/app.h"
#include "../Header/main.h"

// Clamp an integer between two bounds.
static int Cursor_ClampInt(int value, int minValue, int maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

// Reset the cursor visibility and restart the blink timer.
void Cursor_ResetBlink(HWND hWnd, AppState *s)
{
    if (!s)
        return;

    s->cursorVisible = TRUE;
    SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
}

// Stop the blink timer and hide the cursor.
void Cursor_StopBlink(HWND hWnd, AppState *s)
{
    if (!s)
        return;

    KillTimer(hWnd, CURSOR_BLINK_TIMER_ID);
    s->cursorVisible = FALSE;
}

// Restart cursor blinking when the editor regains focus.
LRESULT Cursor_OnSetFocus(HWND hWnd, AppState *s)
{
    if (!s)
        return 0;

    Cursor_ResetBlink(hWnd, s);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

// Stop cursor blinking when the editor loses focus.
LRESULT Cursor_OnKillFocus(HWND hWnd, AppState *s)
{
    if (!s)
        return 0;

    Cursor_StopBlink(hWnd, s);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

// Toggle cursor visibility on the blink timer.
LRESULT Cursor_OnTimer(HWND hWnd, WPARAM wParam, AppState *s)
{
    if (!s)
        return 0;

    if (wParam == CURSOR_BLINK_TIMER_ID)
    {
        s->cursorVisible = !s->cursorVisible;
        InvalidateRect(hWnd, NULL, FALSE);
    }

    return 0;
}

// Clamp the cursor to the valid text area inside the buffer.
void Cursor_SetPosition(TextBuffer *buf, int row, int col)
{
    if (!buf || buf->lineCount <= 0)
    {
        if (buf)
        {
            buf->cursorRow = 0;
            buf->cursorCol = 0;
        }
        return;
    }

    row = Cursor_ClampInt(row, 0, buf->lineCount - 1);
    col = Cursor_ClampInt(col, 0, buf->lineLen[row]);

    buf->cursorRow = row;
    buf->cursorCol = col;
}

// Translate mouse coordinates into a text row and column.
void Cursor_GetPositionFromMouse(LPARAM lParam, const AppState *s, int *outRow, int *outCol)
{
    int row = 0;
    int col = 0;

    if (s)
    {
        int mouseX = (int)(short)LOWORD(lParam) + s->scrollX - TEXT_PADDING_LEFT;
        int mouseY = (int)(short)HIWORD(lParam) + s->scrollY - TEXT_PADDING_TOP;

        row = (s->charHeight ? (mouseY / s->charHeight) : 0);
        row = Cursor_ClampInt(row, 0, s->textBuffer.lineCount - 1);

        col = (s->charWidth ? ((mouseX + s->charWidth / 2) / s->charWidth) : 0);
        if (col < 0)
            col = 0;
        col = Cursor_ClampInt(col, 0, s->textBuffer.lineLen[row]);
    }

    if (outRow)
        *outRow = row;
    if (outCol)
        *outCol = col;
}