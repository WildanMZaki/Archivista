#include "../Header/scroll.h"
#include "../Header/main.h"

void Scroll_EnsureCursorVisible(HWND hWnd)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return;

    RECT rc;
    GetClientRect(hWnd, &rc);

    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    int cursorPixelX = TEXT_PADDING_LEFT + s->textBuffer.cursorCol * s->charWidth;
    int cursorPixelY = TEXT_PADDING_TOP + s->textBuffer.cursorRow * s->charHeight;

    // Vertical scroll
    if (cursorPixelY - s->scrollY < 0)
    {
        s->scrollY = cursorPixelY;
    }
    if (cursorPixelY + s->charHeight - s->scrollY > clientHeight)
    {
        s->scrollY = cursorPixelY + s->charHeight - clientHeight;
    }

    // Horizontal scroll
    if (cursorPixelX - s->scrollX < TEXT_PADDING_LEFT)
    {
        s->scrollX = cursorPixelX - TEXT_PADDING_LEFT;
    }
    if (cursorPixelX - s->scrollX > clientWidth - s->charWidth)
    {
        s->scrollX = cursorPixelX - clientWidth + s->charWidth;
    }

    if (s->scrollX < 0)
        s->scrollX = 0;
    if (s->scrollY < 0)
        s->scrollY = 0;
}

void Scroll_Vertical(AppState *s, WPARAM wParam)
{
    if (!s) return;

    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    s->scrollY -= (delta / WHEEL_DELTA) * s->charHeight * 3;
    if (s->scrollY < 0)
        s->scrollY = 0;
}