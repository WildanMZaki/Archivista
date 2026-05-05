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
  Scroll_EnsureCursorVisible(hWnd);
  InvalidateRect(hWnd, NULL, FALSE);
}

LRESULT App_OnCreate(HWND hWnd)
{
  AppState *s = (AppState *)calloc(1, sizeof(AppState));
  if (!s)
    return -1;

  App_AttachState(hWnd, s);
  Buffer_Init(&s->textBuffer);

  App_SyncEditedState(s);

  HMENU hMenu = CreateAppMenu();
  SetMenu(hWnd, hMenu);

  s->editorFont = CustomFontCanvas(FONT_NAME, FONT_HEIGHT, FONT_WIDTH);
  if (!s->editorFont)
  {
    MessageBox(hWnd, "Font creation failed", "Error", MB_ICONERROR);
    return -1;
  }

  // init state
  s->cursorVisible = TRUE;

  s->scrollX = 0;
  s->scrollY = 0;

  s->selection.active = 0;
  s->selection.start.row = 0;
  s->selection.start.col = 0;
  s->selection.end.row = 0;
  s->selection.end.col = 0;

  Render_CalcCharSize(hWnd);

  Recent_LoadRecent();
  Recent_UpdateMenuRecent(hMenu);

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
    if (!ConfirmSave(hWnd, s))
      return 0;
    PostMessage(hWnd, WM_CLOSE, 0, 0);
    return 0;

  case ID_EDIT_UNDO:
    if (History_CanUndo(&s->history))
    {
      HistoryAction undoAction;
      if (History_Undo(&s->history, &s->textBuffer, &undoAction))
      {
        /* Apply reverse action to buffer */
        Cursor_SetPosition(&s->textBuffer, undoAction.row, undoAction.col);

        if (undoAction.type == HISTORY_ACTION_INSERT)
        {
          /* Reverse INSERT by deleting that text */
          for (int i = 0; i < (int)strlen(undoAction.text); i++)
            Buffer_Delete(&s->textBuffer);
        }
        else if (undoAction.type == HISTORY_ACTION_DELETE)
        {
          /* Reverse DELETE by inserting that text */
          for (int i = 0; undoAction.text[i]; i++)
          {
            if (undoAction.text[i] == '\n')
              Buffer_InsertNewline(&s->textBuffer);
            else
              Buffer_InsertChar(&s->textBuffer, undoAction.text[i]);
          }
        }

        s->selection.active = 0;
        App_SyncEditedState(s);
        App_RefreshEditorAfterAction(hWnd, s);
      }
    }
    return 0;

  case ID_EDIT_REDO:
    if (History_CanRedo(&s->history))
    {
      HistoryAction redoAction;
      if (History_Redo(&s->history, &s->textBuffer, &redoAction))
      {
        /* Apply action to buffer (also reverse, since redoStack contains reversed actions) */
        Cursor_SetPosition(&s->textBuffer, redoAction.row, redoAction.col);

        if (redoAction.type == HISTORY_ACTION_DELETE)
        {
          /* Reverse DELETE by inserting that text */
          for (int i = 0; redoAction.text[i]; i++)
          {
            if (redoAction.text[i] == '\n')
              Buffer_InsertNewline(&s->textBuffer);
            else
              Buffer_InsertChar(&s->textBuffer, redoAction.text[i]);
          }
        }
        else if (redoAction.type == HISTORY_ACTION_INSERT)
        {
          /* Reverse INSERT by deleting that text */
          for (int i = 0; i < (int)strlen(redoAction.text); i++)
            Buffer_Delete(&s->textBuffer);
        }

        s->selection.active = 0;
        App_SyncEditedState(s);
        App_RefreshEditorAfterAction(hWnd, s);
      }
    }
    return 0;

  case ID_EDIT_CUT:
  {
    char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
    if (sel)
    {
      HistoryAction del = History_CreateDeleteAction(sel, s->selection.start.row, s->selection.start.col);
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
    char *sel = Buffer_GetSelectedString(&s->textBuffer, &s->selection);
    if (sel)
    {
      Clipboard_Copy(hWnd, sel);
      free(sel);
    }
    return 0;

  case ID_EDIT_PASTE:
  {
    const char *clipboardText = Clipboard_Paste(hWnd);
    if (!clipboardText)
      return 0; // Nothing to paste

    // Insert string (with selection handling)
    InsertStringResult insertResult = Buffer_InsertString(&s->textBuffer, clipboardText, &s->selection);

    // Push to undo history
    if (insertResult.insertedLen > 0)
    {
      // Create action untuk undo
      // - type = INSERT (untuk undo: delete what was inserted)
      HistoryAction action;
      action.type = HISTORY_ACTION_INSERT;
      action.row = s->textBuffer.cursorRow;
      action.col = s->textBuffer.cursorCol;

      // Copy inserted string ke action.text (truncate jika overflow)
      int copyLen = insertResult.insertedLen;
      if (copyLen > HISTORY_ACTION_BUFFER_SIZE - 1)
        copyLen = HISTORY_ACTION_BUFFER_SIZE - 1;

      if (copyLen > 0)
      {
        memcpy(action.text, insertResult.inserted, (size_t)copyLen);
        action.text[copyLen] = '\0';
      }
      else
      {
        action.text[0] = '\0';
      }

      History_PushAction(&s->history, action);

      // Jika ada removed text (dari selection yg ditimpa), push juga sebagai DELETE action
      if (insertResult.removed && insertResult.removedLen > 0)
      {
        HistoryAction deleteAction;
        deleteAction.type = HISTORY_ACTION_DELETE;

        // Cari start position dari removed text
        // Simplified: assume removes dari selection.start
        deleteAction.row = s->selection.start.row;
        deleteAction.col = s->selection.start.col;

        int copyLen2 = insertResult.removedLen;
        if (copyLen2 > HISTORY_ACTION_BUFFER_SIZE - 1)
          copyLen2 = HISTORY_ACTION_BUFFER_SIZE - 1;

        if (copyLen2 > 0)
        {
          memcpy(deleteAction.text, insertResult.removed, (size_t)copyLen2);
          deleteAction.text[copyLen2] = '\0';
        }
        else
        {
          deleteAction.text[0] = '\0';
        }

        // Push sebagai already-reversed action (since we're recording the deleted content)
        History_PushAction(&s->history, deleteAction);
      }
    }

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
    if (Buffer_DeleteSelection(&s->textBuffer, &s->selection))
    {
      s->selection.active = 0;
    }
    else
    {
      Buffer_Delete(&s->textBuffer);
    }
    App_SyncEditedState(s);
    App_RefreshEditorAfterAction(hWnd, s);
    return 0;

  case ID_EDIT_SELECTALL:
    Selection_SelectAll(s);
    App_RefreshEditorAfterAction(hWnd, s);
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