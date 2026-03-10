#include "../Header/main.h"

HWND hEdit = NULL;
HFONT EditorFont = NULL;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow
                   ) {
    MSG msg;

    if (!InitApplication(hInstance, WinProc))
        return FALSE;

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    BOOL fGotMessage;
    while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WinProc (HWND hWnd,           //Window handle
                          UINT message,        //messages from Windows like WM_DESTROY,WM_PAINT
                          WPARAM wParam,       //Parameters
                          LPARAM lParam)	   //Parameters
{
    switch(message)  //Switch to handle system messages
    {
        case WM_DESTROY:
            DeleteObject(EditorFont);
            PostQuitMessage(0);     //Send the WM_QUIT message to the while loop in WinMAIN()
            break;
        case WM_CREATE:
            hEdit = CreateCanvas(hWnd);
            if (!hEdit)
            {
                MessageBox(NULL,"Failed to create edit control","Error",MB_ICONERROR);
                break;
            }

            EditorFont = CustomFontCanvas(FONT_NAME, FONT_HEIGHT, FONT_WIDTH);
            if (!EditorFont)
            {
                MessageBox(hWnd,"Font creation failed","Error",MB_ICONERROR);
                break;
            }
            SendMessage(hEdit, WM_SETFONT, (WPARAM)EditorFont, TRUE);
            break;
        case WM_SIZE:
            if (hEdit) {
                MoveWindow(hEdit,
                    0,
                    0,
                    LOWORD(lParam),
                    HIWORD(lParam),
                    TRUE);
            }
            break;
        default:
            return DefWindowProc(hWnd,message,wParam,lParam);
    }
    return 0;
}