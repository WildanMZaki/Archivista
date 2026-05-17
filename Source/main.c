#include "../Header/main.h"
#include "../Header/winproc.h"
#include "../Header/search.h"

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    MSG msg;

    Search_Init(); // Inisialisasi Find & Replace

    if (!InitApplication(hInstance, WinProc))
        return FALSE;

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    BOOL fGotMessage;
    while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 && fGotMessage != -1)
    {
        // Mencegah message jika ditujukan untuk dialog Find/Replace (IsDialogMessage)
        // g_searchState bisa dikenali karena sudah di-'extern' di search.h
        if (g_searchState.hDlg == NULL || !IsDialogMessage(g_searchState.hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
