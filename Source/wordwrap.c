#include "../Header/wordwrap.h"
#include "../Header/app.h"
#include "../Header/config.h"
#include "../Header/menu.h"

void WordWrap_Init(HWND hWnd, AppState *s)
{
    if (!s) return;

    // Load from config
    int enabled = Config_ReadInt("Settings", "WordWrap", 0);
    s->wordWrapEnabled = (enabled != 0);
    s->textBuffer.wordWrapEnabled = s->wordWrapEnabled;

    HMENU hMenu = GetMenu(hWnd);
    if (hMenu)
    {
        CheckMenuItem(hMenu, ID_VIEW_WORDWRAP, s->wordWrapEnabled ? MF_CHECKED : MF_UNCHECKED);
    }
}

void WordWrap_Toggle(HWND hWnd, AppState *s)
{
    if (!s) return;

    s->wordWrapEnabled = !s->wordWrapEnabled;
    s->textBuffer.wordWrapEnabled = s->wordWrapEnabled;

    // Update menu checkmark
    HMENU hMenu = GetMenu(hWnd);
    if (hMenu)
    {
        CheckMenuItem(hMenu, ID_VIEW_WORDWRAP, s->wordWrapEnabled ? MF_CHECKED : MF_UNCHECKED);
    }

    // Save to config
    Config_WriteInt("Settings", "WordWrap", s->wordWrapEnabled ? 1 : 0);

    // Recalculate wrapCols
    if (s->wordWrapEnabled)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int clientWidth = rc.right - rc.left;
        s->textBuffer.wrapCols = CalcWrapCols(clientWidth, s->charWidth);
    }
    else
    {
        s->textBuffer.wrapCols = BUF_MAX_COLS - 1;
    }

    // Reflow all lines in the buffer
    Buffer_ReflowAll(&s->textBuffer);

    // Refresh editor UI
    App_RefreshEditorAfterAction(hWnd, s);
}
