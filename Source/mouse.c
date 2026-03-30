#include "../Header/mouse.h"
#include "../Header/main.h"
#include "../Header/app.h"
#include "../Header/selection.h"

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

    int row, col;
    GetTextPositionFromMouse(lParam, s, &row, &col);

    // Update kursor
    s->textBuffer.cursorRow = row;
    s->textBuffer.cursorCol = col;

    // Mulai Seleksi (Selection)
    s->selection.active = 1;
    s->selection.start.row = row;
    s->selection.start.col = col;
    s->selection.end.row = row;
    s->selection.end.col = col;

    // Kunci kursor (supaya tetap terpantau meski mouse keluar aplikasi)
    SetCapture(hWnd);

    Mouse_ResetBlink(hWnd, s);
    SetFocus(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

LRESULT Mouse_OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s) return 0;

    // Cek jika mouse ditarik (dragged) sambil menekan klik kiri
    if ((wParam & MK_LBUTTON) && s->selection.active) {
        int row, col;
        GetTextPositionFromMouse(lParam, s, &row, &col);

        // Update titik akhir seleksi berdasarkan pergerakan kursor
        s->selection.end.row = row;
        s->selection.end.col = col;

        // Kursor pengetikan juga mengikuti arah drag
        s->textBuffer.cursorRow = row;
        s->textBuffer.cursorCol = col;
        Mouse_ResetBlink(hWnd, s);
        InvalidateRect(hWnd, NULL, FALSE);  // Refresh layar untuk menggambar block biru
    }

    return 0;
}

LRESULT Mouse_OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s) return 0;

    // Lepaskan kuncian kursor
    ReleaseCapture();

    if (s->selection.active) {
        // Rekam pergerakan terakhir mouse sepersekian detik sebelum diangkat
        int row, col;
        GetTextPositionFromMouse(lParam, s, &row, &col);

        s->selection.end.row = row;
        s->selection.end.col = col;

        // Cek kembali: jika Start = End sama (cuma klik 1 tempat), lepas block-nya
        if (s->selection.start.row == s->selection.end.row &&
            s->selection.start.col == s->selection.end.col)
        {
            s->selection.active = 0;
        }
    }

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