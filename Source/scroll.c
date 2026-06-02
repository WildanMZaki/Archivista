#include "../Header/scroll.h"
#include "../Header/app.h"
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