#include "../Header/menu.h"

HMENU CreateAppMenu(void)
{
    HMENU hMenuBar = CreateMenu();

    // File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_NEW, "New\tCtrl+N");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, "Open...\tCtrl+O");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE, "Save\tCtrl+S");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVEAS, "Save As...\tCtrl+Shift+S");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "Exit");

    // Edit Menu
    HMENU hEditMenu = CreatePopupMenu();
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_UNDO, "Undo\tCtrl+Z");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_CUT, "Cut\tCtrl+X");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_COPY, "Copy\tCtrl+C");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_PASTE, "Paste\tCtrl+V");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_DELETE, "Delete\tDel");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_SELECTALL, "Select All\tCtrl+A");

    // Help Menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, "About Archivista");

    // Attach to Menu Bar
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, "Edit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");

    return hMenuBar;
}