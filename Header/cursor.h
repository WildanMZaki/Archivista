#ifndef ARCHIVISTA_CURSOR_H
#define ARCHIVISTA_CURSOR_H

#include <windows.h>
#include "buffer.h"

typedef struct AppState AppState;

#define CURSOR_BLINK_TIMER_ID 1
#define CURSOR_BLINK_INTERVAL 500 // milliseconds

// Reset the cursor blink timer and make the cursor visible.
void Cursor_ResetBlink(HWND hWnd, AppState *s);

// Stop the cursor blink timer and hide the cursor.
void Cursor_StopBlink(HWND hWnd, AppState *s);

// Handle focus gain by restarting the cursor blink cycle.
LRESULT Cursor_OnSetFocus(HWND hWnd, AppState *s);

// Handle focus loss by stopping the cursor blink cycle.
LRESULT Cursor_OnKillFocus(HWND hWnd, AppState *s);

// Handle blink timer ticks for the active cursor.
LRESULT Cursor_OnTimer(HWND hWnd, WPARAM wParam, AppState *s);

// Clamp and apply a text cursor position inside the buffer.
void Cursor_SetPosition(TextBuffer *buf, int row, int col);

// Convert a mouse position to a text cursor position.
void Cursor_GetPositionFromMouse(LPARAM lParam, const AppState *s, int *outRow, int *outCol);

#endif // ARCHIVISTA_CURSOR_H