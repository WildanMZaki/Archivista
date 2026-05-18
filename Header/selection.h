#ifndef ARCHIVISTA_SELECTION_H
#define ARCHIVISTA_SELECTION_H

#include "app.h"
#include <windows.h>


void Selection_SelectPoint(AppState *s, int row, int col);
void Selection_SelectLine(AppState *s, int row);
void Selection_SelectWord(AppState *s, int row, int col);
void Selection_SelectAll(AppState *s);
void Selection_SetSelection(AppState *s, int active, int startrow, int startcol, int endrow, int endcol);

#endif // ARCHIVISTA_SELECTION_H