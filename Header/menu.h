#ifndef ARCHIVISTA_MENU_H
#define ARCHIVISTA_MENU_H
#include <windows.h>

// File menu
#define ID_FILE_NEW 1001
#define ID_FILE_OPEN 1002
#define ID_FILE_SAVE 1003
#define ID_FILE_SAVEAS 1004
#define ID_FILE_EXIT 1005

// Edit menu
#define ID_EDIT_UNDO 2001
#define ID_EDIT_REDO 2002
#define ID_EDIT_CUT 2003
#define ID_EDIT_COPY 2004
#define ID_EDIT_PASTE 2005
#define ID_EDIT_DELETE 2006
#define ID_EDIT_SELECTALL 2007

// Help menu
#define ID_HELP_ABOUT 3001

// ========== Function Declarations ==========
HMENU CreateAppMenu(void);

#endif // ARCHIVISTA_MENU_H