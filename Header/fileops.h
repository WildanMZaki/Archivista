#ifndef ARCHIVISTA_FILEOPS_H
#define ARCHIVISTA_FILEOPS_H

#include "../Header/app.h"
#include <windows.h>

void FileOps_New(HWND hWnd, AppState *s);
BOOL FileOps_Open(HWND hWnd, AppState *s, char *path); //add path to open direct file without GetOpenFileName, NULL if want to use GetOpenFileName
BOOL FileOps_Save(HWND hWnd, AppState *s);
BOOL FileOps_SaveAs(HWND hWnd, AppState *s);
BOOL ConfirmSave(HWND hWnd, AppState *s);

#endif // ARCHIVISTA_FILEOPS_H