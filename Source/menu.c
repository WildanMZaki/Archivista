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
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_REDO, "Redo\tCtrl+Y");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_CUT, "Cut\tCtrl+X");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_COPY, "Copy\tCtrl+C");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_PASTE, "Paste\tCtrl+V");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_DELETE, "Delete\tDel");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_FIND, "Find\tCtrl+F");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_REPLACE, "Replace\tCtrl+R");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_GOTO, "Goto\tCtrl+G");
    AppendMenu(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_SELECTALL, "Select All\tCtrl+A");

    // View Menu
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ZOOM_IN, "Zoom In\tCtrl++");
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ZOOM_OUT, "Zoom Out\tCtrl+-");
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ZOOM_RESET, "Reset Zoom\tCtrl+0");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_WORDWRAP, "Word Wrap");

    // Help Menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, "About Archivista");

    // Attach to Menu Bar
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, "Edit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, "View");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");

    return hMenuBar;
}