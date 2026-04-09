#ifndef ARCHIVISTA_APP_H
#define ARCHIVISTA_APP_H

#include "buffer.h"
#include <windows.h>

typedef struct {
  HFONT editorFont;

  BOOL cursorVisible; // blink state
  int charWidth;
  int charHeight;

  // Scroll state (pixels)
  int scrollX;
  int scrollY;

  char currentFilePath[MAX_PATH]; // Menyimpan path file saat ini. Jika string
                                  // kosong, berarti file belum pernah
                                  // disave/Open.

  BOOL isEdited;
  TextBuffer textBuffer;
  TextSelection selection;
} AppState;

// Store/retrieve state via GWLP_USERDATA
void App_AttachState(HWND hWnd, AppState *state);
AppState *App_GetState(HWND hWnd);

LRESULT App_OnCreate(HWND hWnd);
LRESULT App_OnDestroy(HWND hWnd);
LRESULT App_OnClose(HWND hWnd);
LRESULT App_OnSetFocus(HWND hWnd);
LRESULT App_OnKillFocus(HWND hWnd);
LRESULT App_OnTimer(HWND hWnd, WPARAM wParam);

LRESULT App_OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);

#endif