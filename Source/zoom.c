#include "../Header/zoom.h"
#include "../Header/render.h"
#include "../Header/scroll.h"
#include "../Header/config.h"
#include "../Header/main.h"
#include "../Header/wordwrap.h"

void Zoom_Apply(HWND hWnd, AppState *s)
{
    if (!s) return;

    // 1. Hapus font lama
    if (s->editorFont)
    {
        DeleteObject(s->editorFont);
        s->editorFont = NULL;
    }

    // 2. Buat font baru dengan fontSize saat ini
    s->editorFont = CustomFontCanvas(FONT_NAME, s->fontSize, FONT_WIDTH);
    if (!s->editorFont)
    {
        MessageBox(hWnd, "Zoom: Font creation failed", "Error", MB_ICONERROR);
        return;
    }

    // 3. Hitung ulang charWidth dan charHeight berdasarkan font baru
    Render_CalcCharSize(hWnd);
    if (s->wordWrapEnabled)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int clientWidth = rc.right - rc.left;
        s->textBuffer.wrapCols = CalcWrapCols(clientWidth, s->charWidth);
        Buffer_ReflowAll(&s->textBuffer);
    }

    // 4. Pastikan kursor masih terlihat setelah ukuran berubah
    Scroll_EnsureCursorVisible(hWnd);

    // 5. Repaint seluruh area editor
    InvalidateRect(hWnd, NULL, FALSE);

    //Save State to Config
    Config_WriteInt("Settings", "ZoomSize", s->fontSize);
}

void Zoom_In(HWND hWnd, AppState *s)
{
    if (!s) return;

    int newSize = s->fontSize + ZOOM_STEP;
    if (newSize > ZOOM_MAX)
        newSize = ZOOM_MAX;

    // Jangan apply jika tidak ada perubahan
    if (newSize == s->fontSize)
        return;

    s->fontSize = newSize;
    Zoom_Apply(hWnd, s);
}

void Zoom_Out(HWND hWnd, AppState *s)
{
    if (!s) return;

    int newSize = s->fontSize - ZOOM_STEP;
    if (newSize < ZOOM_MIN)
        newSize = ZOOM_MIN;

    // Jangan apply jika tidak ada perubahan
    if (newSize == s->fontSize)
        return;

    s->fontSize = newSize;
    Zoom_Apply(hWnd, s);
}

void Zoom_Reset(HWND hWnd, AppState *s)
{
    if (!s) return;

    // Jangan apply jika sudah default
    if (s->fontSize == ZOOM_DEFAULT)
        return;

    s->fontSize = ZOOM_DEFAULT;
    Zoom_Apply(hWnd, s);
}
