#include "../Header/window.h"

BOOL InitApplication(HINSTANCE hInstance, WNDPROC winProc)
{
    WNDCLASSEX wcx;

    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = winProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = APP_TITLE;
    wcx.hIconSm = LoadImage(hInstance,
                            MAKEINTRESOURCE(5),
                            IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON),
                            GetSystemMetrics(SM_CYSMICON),
                            LR_DEFAULTCOLOR);

    return RegisterClassEx(&wcx);
}

BOOL InitInstance(HINSTANCE hinstance, int nCmdShow)
{
    HWND hwnd;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int posX = (screenWidth - APP_WIDTH) / 2;
    int posY = (screenHeight - APP_HEIGHT) / 2;

    hwnd = CreateWindow(
        APP_TITLE,
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        posX,
        posY,
        APP_WIDTH,
        APP_HEIGHT,
        NULL,
        NULL,
        hinstance,
        NULL);

    if (!hwnd)
        return FALSE;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return TRUE;
}

HFONT CustomFontCanvas(LPCSTR fontName, int fontHeight, int fontWidth)
{
    HFONT hFont = CreateFontA(
        fontHeight, fontWidth, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        fontName);

    return hFont;
}