#include "../Header/app.h"
#include "../Header/fileops.h"
#include "../Header/main.h"
#include "../Header/menu.h"
#include "../Header/render.h"
#include "../Header/scroll.h"
#include <stdlib.h>

void App_AttachState(HWND hWnd, AppState *state) {
  SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state);
}

AppState *App_GetState(HWND hWnd) {
  return (AppState *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

static void App_ResetBlink(HWND hWnd, AppState *s) {
  s->cursorVisible = TRUE;
  SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
}

LRESULT App_OnCreate(HWND hWnd) {
  AppState *s = (AppState *)calloc(1, sizeof(AppState));
  if (!s)
    return -1;

  App_AttachState(hWnd, s);

  Buffer_Init(&s->textBuffer);

  HMENU hMenu = CreateAppMenu();
  SetMenu(hWnd, hMenu);

  s->editorFont = CustomFontCanvas(FONT_NAME, FONT_HEIGHT, FONT_WIDTH);
  if (!s->editorFont) {
    MessageBox(hWnd, "Font creation failed", "Error", MB_ICONERROR);
    return -1;
  }

  s->cursorVisible = TRUE;
  s->scrollX = 0;
  s->scrollY = 0;
  s->selection.active = 0;
  s->selection.start.row = 0;
  s->selection.start.col = 0;
  s->selection.end.row = 0;
  s->selection.end.col = 0;

  Render_CalcCharSize(hWnd);

  // Blink timer mulai ketika focus (behaviour sama seperti sebelumnya:
  // WM_SETFOCUS start)
  return 0;
}

LRESULT App_OnDestroy(HWND hWnd) {
  AppState *s = App_GetState(hWnd);
  KillTimer(hWnd, CURSOR_BLINK_TIMER_ID);

  if (s) {
    Buffer_Free(&s->textBuffer);
    if (s->editorFont)
      DeleteObject(s->editorFont);

    free(s);
    App_AttachState(hWnd, NULL);
  }

  PostQuitMessage(0);
  return 0;
}

LRESULT App_OnSetFocus(HWND hWnd) {
  AppState *s = App_GetState(hWnd);
  if (!s)
    return 0;

  SetTimer(hWnd, CURSOR_BLINK_TIMER_ID, CURSOR_BLINK_INTERVAL, NULL);
  s->cursorVisible = TRUE;
  InvalidateRect(hWnd, NULL, FALSE);
  return 0;
}

LRESULT App_OnKillFocus(HWND hWnd) {
  AppState *s = App_GetState(hWnd);
  if (!s)
    return 0;

  KillTimer(hWnd, CURSOR_BLINK_TIMER_ID);
  s->cursorVisible = FALSE;
  InvalidateRect(hWnd, NULL, FALSE);
  return 0;
}

LRESULT App_OnTimer(HWND hWnd, WPARAM wParam) {
  AppState *s = App_GetState(hWnd);
  if (!s)
    return 0;

  if (wParam == CURSOR_BLINK_TIMER_ID) {
    s->cursorVisible = !s->cursorVisible;
    InvalidateRect(hWnd, NULL, FALSE);
  }
  return 0;
}

LRESULT App_OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
  AppState *s = App_GetState(hWnd);
  if (!s)
    return 0;

  switch (LOWORD(wParam)) {
  case ID_FILE_NEW:
    FileOps_New(hWnd, s);
    return 0;

  case ID_FILE_OPEN:
    FileOps_Open(hWnd, s);
    return 0;

  case ID_FILE_SAVE:
    FileOps_Save(hWnd, s);
    return 0;

  case ID_FILE_SAVEAS:
    FileOps_SaveAs(hWnd, s);
    return 0;

  case ID_FILE_EXIT:
    PostMessage(hWnd, WM_CLOSE, 0, 0);
    return 0;

  case ID_EDIT_UNDO:
    MessageBox(hWnd, "Undo - belum diimplementasi", "Info", MB_OK);
    return 0;

  case ID_EDIT_CUT:
    if (Buffer_DeleteSelection(&s->textBuffer, &s->selection)) {
      s->selection.active = 0;
      App_ResetBlink(hWnd, s);
      Scroll_EnsureCursorVisible(hWnd);
      InvalidateRect(hWnd, NULL, FALSE);
    }
    return 0;

  case ID_EDIT_COPY:
    MessageBox(hWnd, "Copy - belum diimplementasi", "Info", MB_OK);
    return 0;

  case ID_EDIT_PASTE:
    MessageBox(hWnd, "Paste - belum diimplementasi", "Info", MB_OK);
    return 0;

  case ID_EDIT_DELETE:
    if (Buffer_DeleteSelection(&s->textBuffer, &s->selection)) {
      s->selection.active = 0;
    } else {
      Buffer_Delete(&s->textBuffer);
    }
    App_ResetBlink(hWnd, s);
    Scroll_EnsureCursorVisible(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;

  case ID_EDIT_SELECTALL:
    s->selection.active = 1;
    s->selection.start.row = 0;
    s->selection.start.col = 0;
    s->selection.end.row = s->textBuffer.lineCount - 1;
    s->selection.end.col = s->textBuffer.lineLen[s->selection.end.row];

    s->textBuffer.cursorRow = s->selection.end.row;
    s->textBuffer.cursorCol = s->selection.end.col;

    App_ResetBlink(hWnd, s);
    Scroll_EnsureCursorVisible(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;

  case ID_HELP_ABOUT:
    MessageBox(hWnd,
               "Archivista v1.0\n"
               "Simple Text Editor\n\n"
               "Proyek 2 - Teknik Informatika",
               "About Archivista", MB_OK | MB_ICONINFORMATION);
    return 0;
  }

  return 0;
}