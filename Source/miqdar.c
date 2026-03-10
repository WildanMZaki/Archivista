#include "../Header/miqdar.h"

BOOL InitApplication(HINSTANCE hInstance, WNDPROC winProc)
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with parameters
    // that describe the main window.

    wcx.cbSize        =  sizeof(wcx);                   //Size of the structure
    wcx.style         =  CS_HREDRAW | CS_VREDRAW;              //Redraw if Size Changes
    wcx.lpfnWndProc   =  winProc;                     //Points to the Window Procedure" LRESULT CALLBACK WinProc (,,,,);"
    wcx.cbClsExtra    =  0;                                    //No Extra Class Memory
    wcx.cbWndExtra    =  0;                                    //No Extra Window Memory
    wcx.hInstance     =  hInstance;                            //Handle to the instance
    wcx.hIcon         =  LoadIcon(NULL, IDI_APPLICATION);      //predefined app. icon
    wcx.hCursor       =  LoadCursor(NULL,IDC_ARROW);           //Predefined Arrow
    wcx.hbrBackground =  (HBRUSH)GetStockObject(WHITE_BRUSH); //white background brush
    wcx.lpszMenuName  =  NULL;                                 //No Menu
    wcx.lpszClassName =  APP_TITLE;                             //Name of Window Class
    wcx.hIconSm       =  LoadImage(hInstance, // small class icon
        MAKEINTRESOURCE(5),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    return RegisterClassEx(&wcx); //Register the Window class with OS
}

BOOL InitInstance(HINSTANCE hinstance, int nCmdShow)
{
    HWND hwnd;

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN); // Get Screen Width
    int screenHeight = GetSystemMetrics(SM_CYSCREEN); // Get Screen Height

    int posX = (screenWidth  - APP_WIDTH) / 2; // Screen Width - width of the app then divided by 2 to center the window
    int posY = (screenHeight - APP_HEIGHT) / 2; // Screen Height - height of the app then divided by 2 to center the window

    // Create the main window.

    hwnd = CreateWindow(
        APP_TITLE,        // name of window class
        APP_TITLE,            // title-bar string
        WS_OVERLAPPEDWINDOW, // top-level window
        posX,       // default horizontal position
        posY,       // default vertical position
        APP_WIDTH,       // default width
        APP_HEIGHT,       // default height
        NULL,         // no owner window
        NULL,        // use class menu
        hinstance,           // handle to application instance
        NULL);      // no window-creation data

    if (!hwnd)
        return FALSE;

    // Show the window and send a WM_PAINT message to the window
    // procedure.

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return TRUE;

}

HFONT CustomFontCanvas(LPCSTR fontName, int fontHeight, int fontWidth) {
    HFONT hFont = CreateFontA(
    fontHeight, fontWidth, 0, 0,
    FW_NORMAL,
    FALSE, FALSE, FALSE,
    DEFAULT_CHARSET,
    OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,
    CLEARTYPE_QUALITY,
    FF_DONTCARE,
    fontName
    );

    return hFont;
}

HWND CreateCanvas(HWND hWnd) {
    HWND hEdit = CreateWindowEx( // Add canvas text editor from windows using classname EDIT
                WS_EX_CLIENTEDGE,
                "EDIT",
                "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | WS_TABSTOP,
                0,0,0,0,
                hWnd,
                (HMENU)EDIT_CONTROL_ID,
                NULL,
                NULL
            );

    return hEdit;
}