#include <windows.h>
#include "../Header/goto.h"
#include "../Header/cursor.h"

// Subclass procedure for the Edit control to capture Enter and Escape keys
LRESULT CALLBACK GotoEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    GotoDlgContext *ctx = (GotoDlgContext *)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
    if (ctx) {
        if (message == WM_KEYDOWN) {
            if (wParam == VK_RETURN) {
                SendMessage(GetParent(hWnd), WM_COMMAND, IDOK, 0);
                return 0;
            }
        }
    }
    return CallWindowProc(ctx->fnOldEditProc, hWnd, message, wParam, lParam);
}

// Window procedure for the Go To Line dialog
LRESULT CALLBACK GotoDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    GotoDlgContext *ctx = (GotoDlgContext *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message) {
        case WM_CREATE: {
            CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
            ctx = (GotoDlgContext *)pCreate->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
            // 1. Text Prompt
            char promptText[128];
            sprintf(promptText, "Go to line (1 - %d):", ctx->appState->textBuffer.lineCount);
            CreateWindow("STATIC", promptText, WS_CHILD | WS_VISIBLE,
                         15, 15, 250, 20, hWnd, NULL, pCreate->hInstance, NULL);
            // 2. Numeric Input Edit Control (with ES_NUMBER style)
            ctx->hwndEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP,
                                         15, 40, 250, 24, hWnd, NULL, pCreate->hInstance, NULL);

            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(ctx->hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            // Subclass the edit box to catch pressing Enter/Esc
            ctx->fnOldEditProc = (WNDPROC)SetWindowLongPtr(ctx->hwndEdit, GWLP_WNDPROC, (LONG_PTR)GotoEditSubclassProc);
            SetFocus(ctx->hwndEdit);
            // 3. OK Button
            ctx->hwndOkBtn = CreateWindow("BUTTON", "Go To", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                          100, 80, 80, 25, hWnd, (HMENU)IDOK, pCreate->hInstance, NULL);
            SendMessage(ctx->hwndOkBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK) {
                char buffer[32];
                GetWindowText(ctx->hwndEdit, buffer, sizeof(buffer));
                int lineNum = atoi(buffer);

                if (lineNum >= 1 && lineNum <= ctx->appState->textBuffer.lineCount) {
                    int targetRow = lineNum - 1;
                    ctx->appState->selection.active = 0;
                    Cursor_SetPosition(&ctx->appState->textBuffer, targetRow, 0);
                    App_RefreshEditorAfterAction(ctx->hwndOwner, ctx->appState);
                    DestroyWindow(hWnd);
                } else {
                    char errMsg[128];
                    sprintf(errMsg, "Line number must be between 1 and %d.", ctx->appState->textBuffer.lineCount);
                    MessageBox(hWnd, errMsg, "Archivista", MB_OK | MB_ICONWARNING);
                    SetFocus(ctx->hwndEdit);
                    SendMessage(ctx->hwndEdit, EM_SETSEL, 0, -1);
                }
                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            DestroyWindow(hWnd);
            return 0;
        }
        case WM_DESTROY: {
            // Restore control back to the main window
            EnableWindow(ctx->hwndOwner, TRUE);
            SetFocus(ctx->hwndOwner);
            free(ctx);
            return 0;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}