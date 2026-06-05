#include "../Header/mouse.h"
#include "../Header/main.h"
#include "../Header/app.h"
#include "../Header/selection.h"
#include "../Header/cursor.h"
#include "../Header/scroll.h"
#include "../Header/zoom.h"
#include "../Header/zoom.h"

#define MOUSE_AUTOSCROLL_TIMER_ID 2
#define MOUSE_AUTOSCROLL_INTERVAL 50

static DWORD s_lastDblClkTime = 0;
static BOOL s_isWordLineSelection = FALSE;

LRESULT Mouse_OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    int row, col;
    Cursor_GetPositionFromMouse(lParam, s, &row, &col);

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
    SetTimer(hWnd, MOUSE_AUTOSCROLL_TIMER_ID, MOUSE_AUTOSCROLL_INTERVAL, NULL);

    Cursor_ResetBlink(hWnd, s);
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
    Cursor_GetPositionFromMouse(lParam, s, &row, &col);
    Selection_SelectWord(s, row, col);

    SetCapture(hWnd);
    Cursor_ResetBlink(hWnd, s);
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
        Cursor_GetPositionFromMouse(lParam, s, &row, &col);

        // Update titik akhir seleksi berdasarkan pergerakan kursor
        Selection_SetEnd(s, row, col);

        // Kursor pengetikan juga mengikuti arah drag
       Buffer_SetCursorPosition(&s->textBuffer, row, col);
        Cursor_ResetBlink(hWnd, s);
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
    KillTimer(hWnd, MOUSE_AUTOSCROLL_TIMER_ID);

    if (s->selection.active) {
        // Rekam pergerakan terakhir mouse sepersekian detik sebelum diangkat
        // HANYA perbarui koordinat jika ini Dragging biasa
        // (Bukan hasil klik ganda/baris)
        if (!s_isWordLineSelection) {
            int row, col;
            Cursor_GetPositionFromMouse(lParam, s, &row, &col);
            Selection_SetEnd(s, row, col);
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

    // Jika Ctrl ditekan bersamaan → Zoom
    BOOL ctrlDown = (LOWORD(wParam) & MK_CONTROL) != 0;
    if (ctrlDown)
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0)
            Zoom_In(hWnd, s);
        else if (delta < 0)
            Zoom_Out(hWnd, s);
        return 0;
    }

    //Scroll biasa tanpa ctrl
    Scroll_Vertical(hWnd, s, wParam);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

LRESULT Mouse_OnMouseHWheel(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    AppState *s = App_GetState(hWnd);
    if (!s)
        return 0;

    Scroll_Horizontal(hWnd, s, wParam);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

LRESULT Mouse_OnTimer(HWND hWnd, WPARAM wParam, AppState *s)
{
    if (wParam == MOUSE_AUTOSCROLL_TIMER_ID)
    {
        if (s->selection.active && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            // Check if outside horizontally or vertically
            if (pt.y < rc.top || pt.y > rc.bottom || pt.x < rc.left || pt.x > rc.right)
            {
                int row, col;
                LPARAM fakeLParam = MAKELPARAM((short)pt.x, (short)pt.y);
                Cursor_GetPositionFromMouse(fakeLParam, s, &row, &col);
                Selection_SetEnd(s, row, col);
                Buffer_SetCursorPosition(&s->textBuffer, row, col);
                Scroll_EnsureCursorVisible(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        {
            // Failsafe
            KillTimer(hWnd, MOUSE_AUTOSCROLL_TIMER_ID);
        }
        return 0;
    }
    return -1;
}