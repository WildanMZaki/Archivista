#include "../Header/main.h"
#include "../Header/menu.h"
#include "../Header/editor.h"

int FONT_WIDTH;
int FONT_HEIGHT;

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

LRESULT CALLBACK WinProc(HWND hWnd,     // Window handle
                         UINT message,  // messages from Windows like WM_DESTROY,WM_PAINT
                         WPARAM wParam, // Parameters
                         LPARAM lParam) // Parameters
{
    switch (message)
    {
        case WM_DESTROY:
            PostQuitMessage(0); // Send the WM_QUIT message to the while loop in WinMAIN()
            break;

        case WM_CREATE:
        {
            // Setup menu bar
            HMENU hMenu = CreateAppMenu();
            SetMenu(hWnd, hMenu);
            GetFontSize(hWnd);
            InitializeCanvas(hWnd);
            break;
        }

        case WM_PAINT:
            DrawCanvas(hWnd);
            break;
        case WM_CHAR:
            HandleKeyboardTextInput(hWnd, wParam);
            break;
        case WM_KEYDOWN:
            HandleKeyboardforCursor(wParam);
            break;
        case WM_SIZE:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            // ===== File Menu (stub) =====
            case ID_FILE_NEW:
                MessageBox(hWnd, "New file - belum diimplementasi", "Info", MB_OK);
                break;
            case ID_FILE_OPEN:
                MessageBox(hWnd, "Open file - belum diimplementasi", "Info", MB_OK);
                break;
            case ID_FILE_SAVE:
                MessageBox(hWnd, "Save file - belum diimplementasi", "Info", MB_OK);
                break;
            case ID_FILE_SAVEAS:
                MessageBox(hWnd, "Save As - belum diimplementasi", "Info", MB_OK);
                break;
            case ID_FILE_EXIT:
                PostMessage(hWnd, WM_CLOSE, 0, 0);
                break;

            // ===== Edit Menu (stub) =====
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
                MessageBox(hWnd, "Delete - belum diimplementasi", "Info", MB_OK);
                break;
            case ID_EDIT_SELECTALL:
                MessageBox(hWnd, "Select All - belum diimplementasi", "Info", MB_OK);
                break;

            // ===== Help Menu =====
            case ID_HELP_ABOUT:
                MessageBox(hWnd,
                           "Archivista v1.0\n"
                           "Simple Text Editor\n\n"
                           "Proyek 2 - JTK",
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