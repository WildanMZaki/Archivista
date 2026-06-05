#include "../Header/scroll.h"
#include "../Header/main.h"
#include "../Header/utils.h"

static int Scroll_GetLongestLineLen(const TextBuffer *buf)
{
    int longest = 0;
    int lineCount;

    if (!buf)
        return 0;

    lineCount = Buffer_GetLineCount(buf);
    for (int row = 0; row < lineCount; row++)
    {
        int len = Buffer_GetLineLen(buf, row);
        if (len > longest)
            longest = len;
    }

    return longest;
}

static int Scroll_GetMaxScrollY(const AppState *s, int clientHeight)
{
    int lineCount;
    int charHeight;
    int contentHeight;

    if (!s)
        return 0;

    lineCount = Buffer_GetLineCount(&s->textBuffer);
    charHeight = s->charHeight > 0 ? s->charHeight : 1;
    contentHeight = TEXT_PADDING_TOP + (lineCount * charHeight);
    return contentHeight > clientHeight ? contentHeight - clientHeight : 0;
}

static int Scroll_GetContentHeight(const AppState *s)
{
    int lineCount;
    int charHeight;

    if (!s)
        return 0;

    lineCount = Buffer_GetLineCount(&s->textBuffer);
    charHeight = s->charHeight > 0 ? s->charHeight : 1;
    return TEXT_PADDING_TOP + (lineCount * charHeight);
}

static int Scroll_GetMaxScrollX(const AppState *s, int clientWidth)
{
    int longestLen;
    int charWidth;
    int contentWidth;

    if (!s)
        return 0;

    longestLen = Scroll_GetLongestLineLen(&s->textBuffer);
    charWidth = s->charWidth > 0 ? s->charWidth : 1;
    contentWidth = TEXT_PADDING_LEFT + (longestLen * charWidth);
    return contentWidth > clientWidth ? contentWidth - clientWidth + charWidth : 0;
}

static int Scroll_GetContentWidth(const AppState *s)
{
    int longestLen;
    int charWidth;

    if (!s)
        return 0;

    longestLen = Scroll_GetLongestLineLen(&s->textBuffer);
    charWidth = s->charWidth > 0 ? s->charWidth : 1;
    return TEXT_PADDING_LEFT + (longestLen * charWidth);
}

static void Scroll_SetBarInfo(HWND hWnd, int bar, int maxPos, int page, int pos)
{
    SCROLLINFO si;

    if (maxPos < 0)
        maxPos = 0;
    if (page < 1)
        page = 1;
    if (pos < 0)
        pos = 0;
    if (pos > maxPos)
        pos = maxPos;

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = maxPos;
    si.nPage = (UINT)page;
    si.nPos = pos;
    SetScrollInfo(hWnd, bar, &si, TRUE);
}

void Scroll_UpdateScrollbars(HWND hWnd)
{
    AppState *s = App_GetState(hWnd);
    RECT rc;
    int clientWidth;
    int clientHeight;
    int maxScrollX;
    int maxScrollY;
    int contentWidth;
    int contentHeight;
    BOOL showHorz;
    BOOL showVert;

    if (!s)
        return;

    GetClientRect(hWnd, &rc);
    clientWidth = rc.right - rc.left;
    clientHeight = rc.bottom - rc.top;

    contentWidth = Scroll_GetContentWidth(s);
    contentHeight = Scroll_GetContentHeight(s);
    maxScrollX = Scroll_GetMaxScrollX(s, clientWidth);
    maxScrollY = Scroll_GetMaxScrollY(s, clientHeight);
    showHorz = contentWidth > clientWidth;
    showVert = contentHeight > clientHeight;

    s->scrollX = ClampInt(s->scrollX, 0, maxScrollX);
    s->scrollY = ClampInt(s->scrollY, 0, maxScrollY);

    ShowScrollBar(hWnd, SB_HORZ, showHorz);
    ShowScrollBar(hWnd, SB_VERT, showVert);

    Scroll_SetBarInfo(hWnd, SB_HORZ, contentWidth > 0 ? contentWidth - 1 : 0, clientWidth, s->scrollX);
    Scroll_SetBarInfo(hWnd, SB_VERT, contentHeight > 0 ? contentHeight - 1 : 0, clientHeight, s->scrollY);
}

void Scroll_EnsureCursorVisible(HWND hWnd)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return;

    RECT rc;
    GetClientRect(hWnd, &rc);

    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;
    int charWidth = s->charWidth > 0 ? s->charWidth : 1;
    int charHeight = s->charHeight > 0 ? s->charHeight : 1;
    int maxScrollX = Scroll_GetMaxScrollX(s, clientWidth);
    int maxScrollY = Scroll_GetMaxScrollY(s, clientHeight);

    int cursorPixelX = TEXT_PADDING_LEFT + s->textBuffer.cursorCol * charWidth;
    int cursorPixelY = TEXT_PADDING_TOP + s->textBuffer.cursorRow * charHeight;

    // Vertical scroll
    if (cursorPixelY - s->scrollY < 0)
    {
        s->scrollY = cursorPixelY;
    }
    if (cursorPixelY + charHeight - s->scrollY > clientHeight)
    {
        s->scrollY = cursorPixelY + charHeight - clientHeight;
    }

    // Horizontal scroll
    if (cursorPixelX - s->scrollX < TEXT_PADDING_LEFT)
    {
        s->scrollX = cursorPixelX - TEXT_PADDING_LEFT;
    }
    if (cursorPixelX - s->scrollX > clientWidth - charWidth)
    {
        s->scrollX = cursorPixelX - clientWidth + charWidth;
    }

    s->scrollX = ClampInt(s->scrollX, 0, maxScrollX);
    s->scrollY = ClampInt(s->scrollY, 0, maxScrollY);

    Scroll_UpdateScrollbars(hWnd);
}

void Scroll_Vertical(HWND hWnd, AppState *s, WPARAM wParam)
{
    if (!s)
        return;

    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    int step = s->charHeight > 0 ? s->charHeight : 1;

    s->scrollY -= (delta / WHEEL_DELTA) * step * 3;
    Scroll_UpdateScrollbars(hWnd);
}

void Scroll_Horizontal(HWND hWnd, AppState *s, WPARAM wParam)
{
    if (!s)
        return;

    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    int step = s->charWidth > 0 ? s->charWidth : 1;

    s->scrollX += (delta / WHEEL_DELTA) * step * 3;
    Scroll_UpdateScrollbars(hWnd);
}

static void Scroll_HandleBar(HWND hWnd, int bar, WPARAM wParam)
{
    AppState *s = App_GetState(hWnd);
    RECT rc;
    int clientWidth;
    int clientHeight;
    int step;
    int page;
    int maxPos;
    int pos;

    if (!s)
        return;

    GetClientRect(hWnd, &rc);
    clientWidth = rc.right - rc.left;
    clientHeight = rc.bottom - rc.top;

    if (bar == SB_VERT)
    {
        step = s->charHeight > 0 ? s->charHeight : 1;
        page = clientHeight;
        maxPos = Scroll_GetMaxScrollY(s, clientHeight);
        pos = s->scrollY;
    }
    else
    {
        step = s->charWidth > 0 ? s->charWidth : 1;
        page = clientWidth;
        maxPos = Scroll_GetMaxScrollX(s, clientWidth);
        pos = s->scrollX;
    }

    if (bar == SB_VERT)
    {
        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            pos -= step;
            break;

        case SB_LINEDOWN:
            pos += step;
            break;

        case SB_PAGEUP:
            pos -= page;
            break;

        case SB_PAGEDOWN:
            pos += page;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        {
            SCROLLINFO si;
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(hWnd, bar, &si);
            pos = si.nTrackPos;
            break;
        }

        case SB_TOP:
            pos = 0;
            break;

        case SB_BOTTOM:
            pos = maxPos;
            break;

        default:
            return;
        }
    }
    else
    {
        switch (LOWORD(wParam))
        {
        case SB_LINELEFT:
            pos -= step;
            break;

        case SB_LINERIGHT:
            pos += step;
            break;

        case SB_PAGELEFT:
            pos -= page;
            break;

        case SB_PAGERIGHT:
            pos += page;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        {
            SCROLLINFO si;
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(hWnd, bar, &si);
            pos = si.nTrackPos;
            break;
        }

        case SB_LEFT:
            pos = 0;
            break;

        case SB_RIGHT:
            pos = maxPos;
            break;

        default:
            return;
        }
    }

    pos = ClampInt(pos, 0, maxPos);

    if (bar == SB_VERT)
        s->scrollY = pos;
    else
        s->scrollX = pos;

    Scroll_UpdateScrollbars(hWnd);
}

void Scroll_OnVerticalScroll(HWND hWnd, WPARAM wParam)
{
    Scroll_HandleBar(hWnd, SB_VERT, wParam);
}

void Scroll_OnHorizontalScroll(HWND hWnd, WPARAM wParam)
{
    Scroll_HandleBar(hWnd, SB_HORZ, wParam);
}