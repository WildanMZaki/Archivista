#include "../Header/app.h"
#include "../Header/fileops.h"
#include "../Header/cursor.h"
#include "../Header/main.h"
#include "../Header/menu.h"
#include "../Header/render.h"
#include "../Header/scroll.h"
#include "../Header/selection.h"
#include "../Header/recent.h"
#include "../Header/clipboard.h"
#include <string.h>
#include "../Header/search.h"
#include "../Header/zoom.h"
#include "../Header/config.h"
#include "../Header/utils.h"
#include "../Header/window.h"


void App_AttachState(HWND hWnd, AppState *state)
{
  SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state);
}

AppState *App_GetState(HWND hWnd)
{
  return (AppState *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

void App_SyncEditedState(AppState *s)
{
  if (!s)
    return;

  s->isEdited = Buffer_IsBufferChanged(&s->textBuffer);
}

void App_RefreshEditorAfterAction(HWND hWnd, AppState *s)
{
  Cursor_ResetBlink(hWnd, s);
  Scroll_UpdateScrollbars(hWnd);
  Scroll_EnsureCursorVisible(hWnd);
  InvalidateRect(hWnd, NULL, FALSE);
  App_UpdateTitle(hWnd, s);
}

void App_UpdateTitle(HWND hWnd, AppState *s) {
  if (!s) return;

  const char *fileName = "Untitled";
  if (s->currentFilePath[0] != '\0')
  {
    // Extract only the file name from the full path (Windows backslash or standard slash)
    const char *lastBackslash = strrchr(s->currentFilePath, '\\');
    const char *tempName = s->currentFilePath;
    if (lastBackslash != NULL && lastBackslash > tempName)
    {
      tempName = lastBackslash + 1;
    }
    if (tempName[0] != '\0')
    {
      fileName = tempName;
    }
  }
  // 2. Format the new title string
  char titleBuffer[MAX_PATH + 100];
  if (s->isEdited)
  {
    // Option B (with asterisk for modified state)
    wsprintfA(titleBuffer, "*%s - %s", fileName, APP_TITLE);
  }
  else
  {
    // Option A (clean file name)
    wsprintfA(titleBuffer, "%s - %s", fileName, APP_TITLE);
  }
  // 3. Optimize window updates to prevent flickering (only set text if it actually changed)
  char currentTitle[MAX_PATH + 100];
  GetWindowTextA(hWnd, currentTitle, sizeof(currentTitle));
  if (strcmp(currentTitle, titleBuffer) != 0)
  {
    SetWindowTextA(hWnd, titleBuffer);
  }
}

LRESULT App_OnCreate(HWND hWnd)
{
  AppState *s = (AppState *)calloc(1, sizeof(AppState));
  if (!s)
    return -1;

  App_AttachState(hWnd, s);
  Buffer_Init(&s->textBuffer);
  History_Init(&s->history);

  App_SyncEditedState(s);

  HMENU hMenu = CreateAppMenu();
  SetMenu(hWnd, hMenu);

  int loadedSize = Config_ReadInt("Settings", "ZoomSize", ZOOM_DEFAULT);
  if (loadedSize < ZOOM_MIN || loadedSize > ZOOM_MAX) {
    loadedSize = ZOOM_DEFAULT;
  }
  s->fontSize = loadedSize;
  s->editorFont = CustomFontCanvas(FONT_NAME, s->fontSize, FONT_WIDTH);
  if (!s->editorFont)
  {
    MessageBox(hWnd, "Font creation failed", "Error", MB_ICONERROR);
    return -1;
  }

  // init state
  s->cursorVisible = TRUE;

  s->scrollX = 0;
  s->scrollY = 0;

  Selection_SetSelection(s, 0, 0, 0, 0, 0);

  Render_CalcCharSize(hWnd);
  Scroll_UpdateScrollbars(hWnd);

  Recent_LoadRecent();
  Recent_UpdateMenuRecent(hMenu);

  App_UpdateTitle(hWnd, s);

  // Open file from command line argument (e.g. "Open with Archivista" context menu)
  extern char g_cmdLineFilePath[MAX_PATH];
  if (g_cmdLineFilePath[0] != '\0')
  {
    FileOps_Open(hWnd, s, g_cmdLineFilePath);
  }

  // Blink timer mulai ketika focus (behaviour sama seperti sebelumnya:
  // WM_SETFOCUS start)
  return 0;
}

LRESULT App_OnDestroy(HWND hWnd)
{
  AppState *s = App_GetState(hWnd);
  Cursor_StopBlink(hWnd, s);

  if (s)
  {
    History_Free(&s->history);
    Buffer_Free(&s->textBuffer);
    if (s->editorFont)
      DeleteObject(s->editorFont);

    free(s);
    App_AttachState(hWnd, NULL);
  }

  Recent_FreeAllNode();
  PostQuitMessage(0);
  return 0;
}

LRESULT App_OnClose(HWND hWnd)
{
  AppState *s = App_GetState(hWnd);
  if (!ConfirmSave(hWnd, s))
    return 0;
  DestroyWindow(hWnd);
  return 0;
}

LRESULT App_OnSetFocus(HWND hWnd)
{
  AppState *s = App_GetState(hWnd);
  return Cursor_OnSetFocus(hWnd, s);
}

LRESULT App_OnKillFocus(HWND hWnd)
{
  AppState *s = App_GetState(hWnd);
  return Cursor_OnKillFocus(hWnd, s);
}

LRESULT App_OnTimer(HWND hWnd, WPARAM wParam)
{
  AppState *s = App_GetState(hWnd);
  return Cursor_OnTimer(hWnd, wParam, s);
}

LRESULT App_OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  AppState *s = App_GetState(hWnd);
  if (!s)
    return 0;

  switch (LOWORD(wParam))
  {
  case ID_FILE_NEW:
    FileOps_New(hWnd, s);
    return 0;

  case ID_FILE_OPEN:
    FileOps_Open(hWnd, s, NULL);
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
    if (History_CanUndo(&s->history))
    {
      HistoryAction undoAction;
      HistoryAction_Init(&undoAction);
      if (History_Undo(&s->history, &s->textBuffer, &undoAction))
      {
        /* Reverse edits in reverse order */
        for (int i = undoAction.editCount - 1; i >= 0; i--)
        {
          HistoryEdit *edit = &undoAction.edits[i];
          if (edit->type == HISTORY_EDIT_INSERT)
          {
            Cursor_SetPosition(&s->textBuffer, edit->row, edit->col);
            for (int j = 0; j < (int)strlen(edit->text); j++)
              Buffer_Delete(&s->textBuffer);
          }
          else if (edit->type == HISTORY_EDIT_DELETE)
          {
            Cursor_SetPosition(&s->textBuffer, edit->row, edit->col);
            for (int j = 0; edit->text[j]; j++)
            {
              if (edit->text[j] == '\n')
                Buffer_InsertNewline(&s->textBuffer);
              else
                Buffer_InsertChar(&s->textBuffer, edit->text[j]);
            }
          }
        }

        s->selection.active = 0;
        App_SyncEditedState(s);
        App_RefreshEditorAfterAction(hWnd, s);
        HistoryAction_Free(&undoAction);
      }
    }
    return 0;

  case ID_EDIT_REDO:
    if (History_CanRedo(&s->history))
    {
      HistoryAction redoAction;
      HistoryAction_Init(&redoAction);
      if (History_Redo(&s->history, &s->textBuffer, &redoAction))
      {
        /* Apply edits in forward order */
        for (int i = 0; i < redoAction.editCount; i++)
        {
          HistoryEdit *edit = &redoAction.edits[i];
          if (edit->type == HISTORY_EDIT_INSERT)
          {
            Cursor_SetPosition(&s->textBuffer, edit->row, edit->col);
            for (int j = 0; edit->text[j]; j++)
            {
              if (edit->text[j] == '\n')
                Buffer_InsertNewline(&s->textBuffer);
              else
                Buffer_InsertChar(&s->textBuffer, edit->text[j]);
            }
          }
          else if (edit->type == HISTORY_EDIT_DELETE)
          {
            Cursor_SetPosition(&s->textBuffer, edit->row, edit->col);
            for (int j = 0; j < (int)strlen(edit->text); j++)
              Buffer_Delete(&s->textBuffer);
          }
        }

        s->selection.active = 0;
        App_SyncEditedState(s);
        App_RefreshEditorAfterAction(hWnd, s);
        HistoryAction_Free(&redoAction);
      }
    }
    return 0;

  case ID_EDIT_CUT:
  {
    char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
    if (sel)
    {
      TextPos startPos;
      TextPos endPos;

      Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &startPos, &endPos);

      HistoryAction del = History_CreateDeleteAction(sel, startPos.row, startPos.col);
      History_PushAction(&s->history, del);
      Clipboard_Copy(hWnd, sel);
      free(sel);
      Buffer_DeleteSelection(&s->textBuffer, &s->selection);
      s->selection.active = 0;
      App_RefreshEditorAfterAction(hWnd, s);
      App_SyncEditedState(s);
    }
    return 0;
  }

  case ID_EDIT_COPY:
  {
    char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
    if (sel)
    {
      Clipboard_Copy(hWnd, sel);
      free(sel);
    }
    return 0;
  }

  case ID_EDIT_PASTE:
  {
    const char *clipboardText = Clipboard_Paste(hWnd);
    if (!clipboardText)
      return 0; // Nothing to paste

    TextPos pastePos = {s->textBuffer.cursorRow, s->textBuffer.cursorCol};
    if (Buffer_HasSelection(&s->textBuffer, &s->selection))
    {
      TextPos selectionStart;
      TextPos selectionEnd;
      Buffer_NormalizeSelection(&s->textBuffer, &s->selection, &selectionStart, &selectionEnd);
      pastePos = selectionStart;
    }

    // Insert string (with selection handling)
    InsertStringResult insertResult = Buffer_InsertString(&s->textBuffer, clipboardText, &s->selection);

    // Push to undo history - create SINGLE action with both delete and add
    HistoryAction action;
    HistoryAction_Init(&action);

    // Record removed text (dari selection yg ditimpa) sebagai delete part
    if (insertResult.removed && insertResult.removedLen > 0)
    {
      HistoryAction_AddEdit(&action, HISTORY_EDIT_DELETE, insertResult.removed, pastePos.row, pastePos.col, true);
    }

    // Record inserted text sebagai add part
    if (insertResult.insertedLen > 0)
    {
      HistoryAction_AddEdit(&action, HISTORY_EDIT_INSERT, insertResult.inserted, pastePos.row, pastePos.col, false);
    }

    // Push SINGLE action dengan both delete dan add
    History_PushAction(&s->history, action);

    // Cleanup
    Buffer_FreeInsertStringResult(&insertResult);
    free((void *)clipboardText);

    // Update UI
    s->selection.active = 0;
    App_SyncEditedState(s);
    App_RefreshEditorAfterAction(hWnd, s);

    return 0;
  }

  case ID_EDIT_DELETE:
    History_RecordAndExecuteDelete(&s->history, &s->textBuffer, &s->selection);
    App_SyncEditedState(s);
    App_RefreshEditorAfterAction(hWnd, s);
    return 0;

  case ID_EDIT_SELECTALL:
    Selection_SelectAll(s);
    App_RefreshEditorAfterAction(hWnd, s);
    return 0;

  case ID_EDIT_FIND:
      Search_ShowFindDialog(hWnd);
      return 0;

  case ID_EDIT_REPLACE:
      Search_ShowReplaceDialog(hWnd);
      return 0;

  case ID_EDIT_GOTO:
      Search_ShowGotoDialog(hWnd);
      return 0;

  case ID_VIEW_WORDWRAP:
  {
      s->wordWrapEnabled = !s->wordWrapEnabled;
      s->textBuffer.wordWrapEnabled = s->wordWrapEnabled;

      // Update menu checkmark
      HMENU hMenu = GetMenu(hWnd);
      if (hMenu)
      {
          CheckMenuItem(hMenu, ID_VIEW_WORDWRAP, s->wordWrapEnabled ? MF_CHECKED : MF_UNCHECKED);
      }

      // Recalculate wrapCols
      if (s->wordWrapEnabled)
      {
          RECT rc;
          GetClientRect(hWnd, &rc);
          int clientWidth = rc.right - rc.left;
          s->textBuffer.wrapCols = CalcWrapCols(clientWidth, s->charWidth);
      }
      else
      {
          s->textBuffer.wrapCols = BUF_MAX_COLS - 1;
      }

      // Reflow all lines in the buffer
      Buffer_ReflowAll(&s->textBuffer);

      // Refresh editor UI
      App_RefreshEditorAfterAction(hWnd, s);
      return 0;
  }

  case ID_VIEW_ZOOM_IN:
      Zoom_In(hWnd, s);
      return 0;

  case ID_VIEW_ZOOM_OUT:
      Zoom_Out(hWnd, s);
      return 0;

  case ID_VIEW_ZOOM_RESET:
      Zoom_Reset(hWnd, s);
      return 0;

  case ID_HELP_ABOUT:
    MessageBox(hWnd,
               "Archivista v1.0\n"
               "Simple Text Editor\n\n"
               "Proyek 2 - Teknik Informatika",
               "About Archivista", MB_OK | MB_ICONINFORMATION);
    return 0;

  default:
    if (LOWORD(wParam) >= ID_FILE_RECENT_START && LOWORD(wParam) <= ID_FILE_RECENT_END)
    {
      int idIndex = LOWORD(wParam) - ID_FILE_RECENT_START;
      FileOps_Open(hWnd, s, Recent_GetRecentPathByIndex(idIndex));
      return 0;
    }
  }

  return 0;
}