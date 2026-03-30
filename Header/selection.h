#ifndef ARCHIVISTA_SELECTION_H
#define ARCHIVISTA_SELECTION_H

#include <windows.h>
#include "app.h"

void Selection_SelectPoint(AppState *s, int row, int col);
void Selection_SelectLine(AppState *s, int row);
void Selection_SelectWord(AppState *s, int row, int col);

#endif //ARCHIVISTA_SELECTION_H