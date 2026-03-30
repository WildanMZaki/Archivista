#ifndef ARCHIVISTA_SELECTION_H
#define ARCHIVISTA_SELECTION_H

#include <windows.h>
#include "../Header/app.h"

void GetTextPositionFromMouse(LPARAM lParam, AppState *s, int *outRow, int *outCol);

#endif //ARCHIVISTA_SELECTION_H