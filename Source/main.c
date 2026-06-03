#include "../Header/main.h"
#include "../Header/winproc.h"
#include "../Header/search.h"
#include <string.h>

// Global: file path from command line argument (for "Open with" context menu)
char g_cmdLineFilePath[MAX_PATH] = {0};

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    MSG msg;

    // Parse command line: strip surrounding quotes if present
    if (lpCmdLine && lpCmdLine[0] != '\0')
    {
        const char *src = lpCmdLine;
        // Skip leading whitespace
        while (*src == ' ') src++;
        // Strip surrounding quotes
        size_t len = strlen(src);
        if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
        {
            strncpy(g_cmdLineFilePath, src + 1, len - 2);
            g_cmdLineFilePath[len - 2] = '\0';
        }
        else
        {
            strncpy(g_cmdLineFilePath, src, MAX_PATH - 1);
            g_cmdLineFilePath[MAX_PATH - 1] = '\0';
        }
    }

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
