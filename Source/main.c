#include "../Header/main.h"
#include "../Header/menu.h"
#include "../Header/buffer.h"

HFONT EditorFont = NULL;
TextBuffer textBuffer;     // Array 2D — source of truth
BOOL cursorVisible = TRUE; // Cursor blink state
int charWidth = 0;         // Lebar 1 karakter (monospace)
int charHeight = 0;        // Tinggi 1 karakter

// Scroll state
int scrollX = 0; // Horizontal scroll offset (in pixels)
int scrollY = 0; // Vertical scroll offset (in pixels)

// =============================================================
//  Helper: Hitung ukuran 1 karakter dari font
// =============================================================

void CalculateCharSize(HWND hWnd)
{
    HDC hdc = GetDC(hWnd);
    HFONT oldFont = (HFONT)SelectObject(hdc, EditorFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    charWidth = tm.tmAveCharWidth;
    charHeight = tm.tmHeight + tm.tmExternalLeading;

    SelectObject(hdc, oldFont);
    ReleaseDC(hWnd, hdc);
}

// =============================================================
//  Helper: Ensure cursor is visible (auto-scroll)
// =============================================================

void EnsureCursorVisible(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);

    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    // Posisi cursor dalam pixel
    int cursorPixelX = TEXT_PADDING_LEFT + textBuffer.cursorCol * charWidth;
    int cursorPixelY = TEXT_PADDING_TOP + textBuffer.cursorRow * charHeight;

    // Vertical scroll
    if (cursorPixelY - scrollY < 0)
    {
        scrollY = cursorPixelY;
    }
    if (cursorPixelY + charHeight - scrollY > clientHeight)
    {
        scrollY = cursorPixelY + charHeight - clientHeight;
    }

    // Horizontal scroll
    if (cursorPixelX - scrollX < TEXT_PADDING_LEFT)
    {
        scrollX = cursorPixelX - TEXT_PADDING_LEFT;
    }
    if (cursorPixelX - scrollX > clientWidth - charWidth)
    {
        scrollX = cursorPixelX - clientWidth + charWidth;
    }

    if (scrollX < 0)
        scrollX = 0;
    if (scrollY < 0)
        scrollY = 0;
}

// =============================================================
//  WinMain
// =============================================================

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    MSG msg;

    if (!InitApplication(hInstance, WinProc))
        return FALSE;

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    BOOL fGotMessage;
    while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 && fGotMessage != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// =============================================================
//  Window Procedure
// =============================================================

LRESULT CALLBACK WinProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam)
{
    switch (message)
    {
        // =========================================================
        //  LIFECYCLE
        // =========================================================

    case WM_CREATE:
    {
        Buffer_Init(&textBuffer);

        HMENU hMenu = CreateAppMenu();
        SetMenu(hWnd, hMenu);

        EditorFont = CustomFontCanvas(FONT_NAME, FONT_HEIGHT, FONT_WIDTH);
        if (!EditorFont)
        {
            MessageBox(hWnd, "Font creation failed", "Error", MB_ICONERROR);
            break;
        }

        CalculateCharSize(hWnd);
        break;
    }

    case WM_DESTROY:
        KillTimer(hWnd, CURSOR_BLINK_TIMER_ID);
        Buffer_Free(&textBuffer);
        DeleteObject(EditorFont);
        PostQuitMessage(0);
        break;

        // =========================================================
        //  FOCUS & CURSOR BLINKING
        // =========================================================

    case WM_SETFOCUS:
        // Window dapat focus → mulai blink cursor
        SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
        cursorVisible = TRUE;
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_KILLFOCUS:
        // Window kehilangan focus → matikan cursor
        KillTimer(hWnd, CURSOR_BLINK_TIMER_ID);
        cursorVisible = FALSE;
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_TIMER:
        if (wParam == CURSOR_BLINK_TIMER_ID)
        {
            cursorVisible = !cursorVisible;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

        // =========================================================
        //  PAINTING — Gambar teks dari buffer ke window
        // =========================================================

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Double buffering: gambar ke memory DC dulu, baru copy ke layar
        // Ini mencegah flickering
        RECT rc;
        GetClientRect(hWnd, &rc);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        // Background putih
        FillRect(memDC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

        // Set font
        HFONT oldFont = (HFONT)SelectObject(memDC, EditorFont);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(0, 0, 0));

        // Gambar setiap baris dari buffer
        for (int i = 0; i < textBuffer.lineCount; i++)
        {
            int x = TEXT_PADDING_LEFT - scrollX;
            int y = TEXT_PADDING_TOP + (i * charHeight) - scrollY;

            // Skip baris yang di luar viewport (optimasi)
            if (y + charHeight < 0)
                continue;
            if (y > rc.bottom)
                break;

            if (textBuffer.lines[i].length > 0)
            {
                TextOutA(memDC, x, y,
                         textBuffer.lines[i].text,
                         textBuffer.lines[i].length);
            }
        }

        // Gambar cursor (garis vertikal)
        if (cursorVisible)
        {
            int cursorX = TEXT_PADDING_LEFT + textBuffer.cursorCol * charWidth - scrollX;
            int cursorY = TEXT_PADDING_TOP + textBuffer.cursorRow * charHeight - scrollY;

            // Garis vertikal hitam, tebal 2px
            HPEN cursorPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            HPEN oldPen = (HPEN)SelectObject(memDC, cursorPen);
            MoveToEx(memDC, cursorX, cursorY, NULL);
            LineTo(memDC, cursorX, cursorY + charHeight);
            SelectObject(memDC, oldPen);
            DeleteObject(cursorPen);
        }

        // Copy memory DC ke layar
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(memDC, oldFont);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        break;
    }

        // =========================================================
        //  KEYBOARD — Karakter (huruf, angka, enter, backspace)
        // =========================================================

    case WM_CHAR:
    {
        char c = (char)wParam;

        if (c == '\r')
        {
            // Enter key
            Buffer_InsertNewline(&textBuffer);
        }
        else if (c == '\b')
        {
            // Backspace
            Buffer_Backspace(&textBuffer);
        }
        else if (c == '\t')
        {
            // Tab → insert 4 spaces
            for (int i = 0; i < 4; i++)
            {
                Buffer_InsertChar(&textBuffer, ' ');
            }
        }
        else if (c >= 32)
        {
            // Printable character
            Buffer_InsertChar(&textBuffer, c);
        }

        // Reset cursor blink (supaya keliatan pas ngetik)
        cursorVisible = TRUE;
        SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);

        EnsureCursorVisible(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

        // =========================================================
        //  KEYBOARD — Special keys (arrows, home, end, dll)
        // =========================================================

    case WM_KEYDOWN:
    {
        Line *currentLine = &textBuffer.lines[textBuffer.cursorRow];

        switch (wParam)
        {
        case VK_LEFT:
            if (textBuffer.cursorCol > 0)
            {
                textBuffer.cursorCol--;
            }
            else if (textBuffer.cursorRow > 0)
            {
                // Pindah ke akhir baris sebelumnya
                textBuffer.cursorRow--;
                textBuffer.cursorCol = textBuffer.lines[textBuffer.cursorRow].length;
            }
            break;

        case VK_RIGHT:
            if (textBuffer.cursorCol < currentLine->length)
            {
                textBuffer.cursorCol++;
            }
            else if (textBuffer.cursorRow < textBuffer.lineCount - 1)
            {
                // Pindah ke awal baris berikutnya
                textBuffer.cursorRow++;
                textBuffer.cursorCol = 0;
            }
            break;

        case VK_UP:
            if (textBuffer.cursorRow > 0)
            {
                textBuffer.cursorRow--;
                // Clamp col supaya gak melebihi panjang baris tujuan
                if (textBuffer.cursorCol > textBuffer.lines[textBuffer.cursorRow].length)
                {
                    textBuffer.cursorCol = textBuffer.lines[textBuffer.cursorRow].length;
                }
            }
            break;

        case VK_DOWN:
            if (textBuffer.cursorRow < textBuffer.lineCount - 1)
            {
                textBuffer.cursorRow++;
                if (textBuffer.cursorCol > textBuffer.lines[textBuffer.cursorRow].length)
                {
                    textBuffer.cursorCol = textBuffer.lines[textBuffer.cursorRow].length;
                }
            }
            break;

        case VK_HOME:
            textBuffer.cursorCol = 0;
            break;

        case VK_END:
            textBuffer.cursorCol = currentLine->length;
            break;

        case VK_PRIOR: // Page Up
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int pageLines = (rc.bottom - rc.top) / charHeight;
            textBuffer.cursorRow -= pageLines;
            if (textBuffer.cursorRow < 0)
                textBuffer.cursorRow = 0;
            if (textBuffer.cursorCol > textBuffer.lines[textBuffer.cursorRow].length)
            {
                textBuffer.cursorCol = textBuffer.lines[textBuffer.cursorRow].length;
            }
            break;
        }

        case VK_NEXT: // Page Down
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int pageLines = (rc.bottom - rc.top) / charHeight;
            textBuffer.cursorRow += pageLines;
            if (textBuffer.cursorRow >= textBuffer.lineCount)
            {
                textBuffer.cursorRow = textBuffer.lineCount - 1;
            }
            if (textBuffer.cursorCol > textBuffer.lines[textBuffer.cursorRow].length)
            {
                textBuffer.cursorCol = textBuffer.lines[textBuffer.cursorRow].length;
            }
            break;
        }

        case VK_DELETE:
            Buffer_Delete(&textBuffer);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        cursorVisible = TRUE;
        SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);

        EnsureCursorVisible(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

        // =========================================================
        //  MOUSE — Klik untuk posisi cursor
        // =========================================================

    case WM_LBUTTONDOWN:
    {
        int mouseX = LOWORD(lParam) + scrollX - TEXT_PADDING_LEFT;
        int mouseY = HIWORD(lParam) + scrollY - TEXT_PADDING_TOP;

        // Hitung baris dari posisi mouse
        int row = mouseY / charHeight;
        if (row < 0)
            row = 0;
        if (row >= textBuffer.lineCount)
            row = textBuffer.lineCount - 1;

        // Hitung kolom dari posisi mouse
        int col = (mouseX + charWidth / 2) / charWidth; // +half char for snapping
        if (col < 0)
            col = 0;
        if (col > textBuffer.lines[row].length)
            col = textBuffer.lines[row].length;

        textBuffer.cursorRow = row;
        textBuffer.cursorCol = col;

        cursorVisible = TRUE;
        SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);

        SetFocus(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

        // =========================================================
        //  MOUSE WHEEL — Scroll
        // =========================================================

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        scrollY -= (delta / WHEEL_DELTA) * charHeight * 3; // 3 baris per scroll
        if (scrollY < 0)
            scrollY = 0;

        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

        // =========================================================
        //  MENU COMMANDS
        // =========================================================

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_NEW:
            Buffer_Clear(&textBuffer);
            scrollX = 0;
            scrollY = 0;
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case ID_FILE_OPEN:
            MessageBox(hWnd, "Open file - butuh fileops module", "Info", MB_OK);
            break;

        case ID_FILE_SAVE:
            MessageBox(hWnd, "Save file - butuh fileops module", "Info", MB_OK);
            break;

        case ID_FILE_SAVEAS:
            MessageBox(hWnd, "Save As - butuh fileops module", "Info", MB_OK);
            break;

        case ID_FILE_EXIT:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case ID_EDIT_UNDO:
            MessageBox(hWnd, "Undo - belum diimplementasi", "Info", MB_OK);
            break;

        case ID_EDIT_CUT:
            MessageBox(hWnd, "Cut - belum diimplementasi", "Info", MB_OK);
            break;

        case ID_EDIT_COPY:
            MessageBox(hWnd, "Copy - belum diimplementasi", "Info", MB_OK);
            break;

        case ID_EDIT_PASTE:
            MessageBox(hWnd, "Paste - belum diimplementasi", "Info", MB_OK);
            break;

        case ID_EDIT_DELETE:
            Buffer_Delete(&textBuffer);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case ID_EDIT_SELECTALL:
            MessageBox(hWnd, "Select All - belum diimplementasi", "Info", MB_OK);
            break;

        case ID_HELP_ABOUT:
            MessageBox(hWnd,
                       "Archivista v1.0\n"
                       "Simple Text Editor\n\n"
                       "Proyek 2 - Teknik Informatika",
                       "About Archivista",
                       MB_OK | MB_ICONINFORMATION);
            break;
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}