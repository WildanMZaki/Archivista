#include "../Header/editor.h"
#include "../Header/main.h"

Editor editor;

void InitializeCanvas(HWND hWnd) {
    editor.lineCount = 1;
    editor.cursorX = 0;
    editor.cursorY = 0;

    CreateCaret(hWnd, NULL, 2, FONT_HEIGHT);
    ShowCaret(hWnd);
}

void HandleKeyboardTextInput(HWND hWnd, WPARAM wParam) {
    char c = (char)wParam;

    if (c == '\r')
    {
        editor.cursorY++;
        editor.cursorX = 0;

        if (editor.cursorY >= editor.lineCount)
            editor.lineCount = editor.cursorY + 1;
    }
    else if (c == '\b') {
        // Handle backspace
        if (editor.cursorX > 0) {
            editor.cursorX--;
            editor.text[editor.cursorY][editor.cursorX] = '\0';
        }
    }
    else if ((unsigned char)c >= 32 && c != 127) {
        // Only printable ASCII (space and above, excluding DEL)
        editor.text[editor.cursorY][editor.cursorX++] = c;
        editor.text[editor.cursorY][editor.cursorX] = '\0';
    }
    else
    {
        return;
    }

    SetCaretPos(editor.cursorX * FONT_WIDTH + 5, editor.cursorY * FONT_HEIGHT);

    InvalidateRect(hWnd, NULL, TRUE);
}

void DrawCanvas(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width  = clientRect.right  - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // Create off-screen buffer
    HDC     memDC  = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // Clear background on the buffer (not the screen)
    FillRect(memDC, &clientRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

    // Draw text onto the buffer
    for (int i = 0; i < editor.lineCount; i++) {
        TextOutA(memDC, 5, i * FONT_HEIGHT, editor.text[i], strlen(editor.text[i]));
    }

    // Blit the whole buffer to screen in one shot — no flicker
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
}

BOOL HandleKeyboardforCursor(WPARAM wParam) {
    // Only handle navigation keys — ignore all others
    if (wParam != VK_LEFT && wParam != VK_RIGHT &&
        wParam != VK_UP   && wParam != VK_DOWN)
        return FALSE;

    int lineLen = (int)strlen(editor.text[editor.cursorY]);

    switch (wParam)
    {
        case VK_LEFT:
            if (editor.cursorX > 0) editor.cursorX--;
            break;

        case VK_RIGHT:
            if (editor.cursorX < lineLen) editor.cursorX++;
            break;

        case VK_UP:
            if (editor.cursorY > 0) {
                editor.cursorY--;
                int newLen = (int)strlen(editor.text[editor.cursorY]);
                if (editor.cursorX > newLen) editor.cursorX = newLen;
            }
            break;

        case VK_DOWN:
            if (editor.cursorY < editor.lineCount - 1) {
                editor.cursorY++;
                int newLen = (int)strlen(editor.text[editor.cursorY]);
                if (editor.cursorX > newLen) editor.cursorX = newLen;
            }
            break;
    }

    BOOL isSuccess = SetCaretPos(editor.cursorX * FONT_WIDTH + 5, editor.cursorY * FONT_HEIGHT);
    return isSuccess;
}

void GetFontSize(HWND hWnd) {
    HDC hdc = GetDC(hWnd);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    FONT_WIDTH = tm.tmAveCharWidth;
    FONT_HEIGHT = tm.tmHeight;

    ReleaseDC(hWnd, hdc);
}