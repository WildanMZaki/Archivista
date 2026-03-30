#include "../Header/render.h"
#include "../Header/app.h"
#include "../Header/main.h"

void Render_CalcCharSize(HWND hWnd)
{
    AppState *s = App_GetState(hWnd);
    if (!s || !s->editorFont)
        return;

    HDC hdc = GetDC(hWnd);
    HFONT oldFont = (HFONT)SelectObject(hdc, s->editorFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    s->charWidth = tm.tmAveCharWidth;
    s->charHeight = tm.tmHeight + tm.tmExternalLeading;

    SelectObject(hdc, oldFont);
    ReleaseDC(hWnd, hdc);
}

LRESULT Render_OnPaint(HWND hWnd)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rc;
    GetClientRect(hWnd, &rc);

    // Double buffering (anti flicker)
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    FillRect(memDC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

    HFONT oldFont = (HFONT)SelectObject(memDC, s->editorFont);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(0, 0, 0));

    TextPos selStart = {0, 0};
    TextPos selEnd = {0, 0};
    int hasSelection = Buffer_HasSelection(&s->textBuffer, &s->selection);
    if (hasSelection)
    {
        Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selStart, &selEnd);
    }

    // Draw lines
    for (int i = 0; i < s->textBuffer.lineCount; i++)
    {
        int x = TEXT_PADDING_LEFT - s->scrollX;
        int y = TEXT_PADDING_TOP + (i * s->charHeight) - s->scrollY;

        if (y + s->charHeight < 0)
            continue;
        if (y > rc.bottom)
            break;

        int len = s->textBuffer.lineLen[i];
        int selColStart = -1;
        int selColEnd = -1;

        if (hasSelection && i >= selStart.row && i <= selEnd.row)
        {
            if (selStart.row == selEnd.row)
            {
                selColStart = selStart.col;
                selColEnd = selEnd.col;
            }
            else if (i == selStart.row)
            {
                selColStart = selStart.col;
                selColEnd = len;
            }
            else if (i == selEnd.row)
            {
                selColStart = 0;
                selColEnd = selEnd.col;
            }
            else
            {
                selColStart = 0;
                selColEnd = len;
            }

            if (selColStart < 0)
                selColStart = 0;
            if (selColEnd > len)
                selColEnd = len;
            if (selColEnd < selColStart)
                selColEnd = selColStart;
        }

        if (selColStart >= 0 && selColEnd > selColStart)
        {
            RECT selRect;
            selRect.left = x + (selColStart * s->charWidth);
            selRect.top = y;
            selRect.right = x + (selColEnd * s->charWidth);
            selRect.bottom = y + s->charHeight;

            HBRUSH selBrush = CreateSolidBrush(RGB(0, 120, 215));
            FillRect(memDC, &selRect, selBrush);
            DeleteObject(selBrush);

            if (selColStart > 0)
            {
                SetTextColor(memDC, RGB(0, 0, 0));
                TextOutA(memDC, x, y, s->textBuffer.lines[i], selColStart);
            }

            SetTextColor(memDC, RGB(255, 255, 255));
            TextOutA(memDC,
                     x + (selColStart * s->charWidth),
                     y,
                     &s->textBuffer.lines[i][selColStart],
                     selColEnd - selColStart);

            if (selColEnd < len)
            {
                SetTextColor(memDC, RGB(0, 0, 0));
                TextOutA(memDC,
                         x + (selColEnd * s->charWidth),
                         y,
                         &s->textBuffer.lines[i][selColEnd],
                         len - selColEnd);
            }
        }
        else if (len > 0)
        {
            SetTextColor(memDC, RGB(0, 0, 0));
            TextOutA(memDC, x, y, s->textBuffer.lines[i], len);
        }
    }

    // Draw cursor
    if (s->cursorVisible)
    {
        int cursorX = TEXT_PADDING_LEFT + s->textBuffer.cursorCol * s->charWidth - s->scrollX;
        int cursorY = TEXT_PADDING_TOP + s->textBuffer.cursorRow * s->charHeight - s->scrollY;

        HPEN cursorPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(memDC, cursorPen);
        MoveToEx(memDC, cursorX, cursorY, NULL);
        LineTo(memDC, cursorX, cursorY + s->charHeight);
        SelectObject(memDC, oldPen);
        DeleteObject(cursorPen);
    }

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldFont);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
    return 0;
}