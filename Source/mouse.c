#include "../Header/mouse.h"
#include "../Header/main.h"
#include "../Header/app.h"
#include "../Header/selection.h"

static DWORD s_lastDblClkTime = 0;
static BOOL s_isWordLineSelection = FALSE;

static void Mouse_ResetBlink(HWND hWnd, AppState *s)
{
    s->cursorVisible = TRUE;
    SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
}

static void GetTextPositionFromMouse(LPARAM lParam, AppState *s, int *outRow, int *outCol) {
    int mouseX = (int)(short)LOWORD(lParam) + s->scrollX - TEXT_PADDING_LEFT;
    int mouseY = (int)(short)HIWORD(lParam) + s->scrollY - TEXT_PADDING_TOP;
    int row = (s->charHeight ? (mouseY / s->charHeight) : 0);
    if (row < 0) row = 0;
    if (row >= s->textBuffer.lineCount) row = s->textBuffer.lineCount - 1;
    int col = (s->charWidth ? ((mouseX + s->charWidth / 2) / s->charWidth) : 0);
    if (col < 0) col = 0;

    int len = s->textBuffer.lineLen[row];
    if (col > len) col = len;
    *outRow = row;
    *outCol = col;
}

LRESULT Mouse_OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    int row, col;
    GetTextPositionFromMouse(lParam, s, &row, &col);

    // Ambil waktu saat klik ini ditekan
    DWORD now = GetMessageTime();

    // Jika jarak waktu klik ini dengan Double Click terakhir masih dalam rentang
    // kecepatan double-click Windows, maka ini adalah TRIPLE CLICK!
    if (now - s_lastDblClkTime <= GetDoubleClickTime()) {
        s_isWordLineSelection = TRUE;
        Selection_SelectLine(s, row);
        s_lastDblClkTime = 0; // Reset agar tidak terjadi bug quadruple click beruntun
    }
    else {
        // Jika bukan triple click, lakukan seleksi normal klik sekali (Tarik/Drag)
        s_isWordLineSelection = FALSE;
        Selection_SelectPoint(s, row, col);
    }

    // Kunci kursor (supaya tetap terpantau meski mouse keluar aplikasi)
    SetCapture(hWnd);

    Mouse_ResetBlink(hWnd, s);
    SetFocus(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

LRESULT Mouse_OnLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // Catat waktu kapan klik ganda dieksekusi untuk dipantau oleh Triple Click
    s_lastDblClkTime = GetMessageTime();
    AppState *s = App_GetState(hWnd);
    if (!s) return 0;
    s_isWordLineSelection = TRUE;
    int row, col;
    GetTextPositionFromMouse(lParam, s, &row, &col);
    Selection_SelectWord(s, row, col);

    SetCapture(hWnd);
    Mouse_ResetBlink(hWnd, s);
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
        // HANYA perbarui koordinat jika ini Dragging biasa
        // (Bukan hasil klik ganda/baris)
        if (!s_isWordLineSelection) {
            int row, col;
            GetTextPositionFromMouse(lParam, s, &row, &col);
            s->selection.end.row = row;
            s->selection.end.col = col;
        }

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