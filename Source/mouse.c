#include "../Header/mouse.h"
#include "../Header/main.h"
#include "../Header/app.h"

static void Mouse_ResetBlink(HWND hWnd, AppState *s)
{
    s->cursorVisible = TRUE;
    SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
}

LRESULT Mouse_OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    int mouseX = LOWORD(lParam) + s->scrollX - TEXT_PADDING_LEFT;
    int mouseY = HIWORD(lParam) + s->scrollY - TEXT_PADDING_TOP;

    int row = (s->charHeight ? (mouseY / s->charHeight) : 0);
    if (row < 0)
        row = 0;
    if (row >= s->textBuffer.lineCount)
        row = s->textBuffer.lineCount - 1;

    int col = (s->charWidth ? ((mouseX + s->charWidth / 2) / s->charWidth) : 0);
    if (col < 0)
        col = 0;
    int len = s->textBuffer.lineLen[row];
    if (col > len)
        col = len;

    s->textBuffer.cursorRow = row;
    s->textBuffer.cursorCol = col;

    Mouse_ResetBlink(hWnd, s);
    SetFocus(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

LRESULT Mouse_OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    s->scrollY -= (delta / WHEEL_DELTA) * s->charHeight * 3;
    if (s->scrollY < 0)
        s->scrollY = 0;

    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}