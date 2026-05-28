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

  s->fontSize = ZOOM_DEFAULT;
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

  Selection_SetSelection(s, 0, 0, 0, 0, 0);

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
    PostMessage(hWnd, WM_CLOSE, 0, 0);
    return 0;

  case ID_EDIT_UNDO:
    if (History_CanUndo(&s->history))
    {
      HistoryAction undoAction;
      if (History_Undo(&s->history, &s->textBuffer, &undoAction))
      {
        /* Reverse add: DELETE what was added */
        if (undoAction.add.active && undoAction.add.text[0] != '\0')
        {
          Cursor_SetPosition(&s->textBuffer, undoAction.add.row, undoAction.add.col);
          for (int i = 0; i < (int)strlen(undoAction.add.text); i++)
            Buffer_Delete(&s->textBuffer);
        }

        /* Reverse delete: INSERT what was deleted */
        if (undoAction.delete.active && undoAction.delete.text[0] != '\0')
        {
          Cursor_SetPosition(&s->textBuffer, undoAction.delete.row, undoAction.delete.col);
          for (int i = 0; undoAction.delete.text[i]; i++)
          {
            if (undoAction.delete.text[i] == '\n')
              Buffer_InsertNewline(&s->textBuffer);
            else
              Buffer_InsertChar(&s->textBuffer, undoAction.delete.text[i]);
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
        /* Redo uses same logic as undo: DELETE add, INSERT delete */
        /* Reverse add: DELETE what was added */
        if (redoAction.add.active && redoAction.add.text[0] != '\0')
        {
          Cursor_SetPosition(&s->textBuffer, redoAction.add.row, redoAction.add.col);
          for (int i = 0; i < (int)strlen(redoAction.add.text); i++)
            Buffer_Delete(&s->textBuffer);
        }

        /* Reverse delete: INSERT what was deleted */
        if (redoAction.delete.active && redoAction.delete.text[0] != '\0')
        {
          Cursor_SetPosition(&s->textBuffer, redoAction.delete.row, redoAction.delete.col);
          for (int i = 0; redoAction.delete.text[i]; i++)
          {
            if (redoAction.delete.text[i] == '\n')
              Buffer_InsertNewline(&s->textBuffer);
            else
              Buffer_InsertChar(&s->textBuffer, redoAction.delete.text[i]);
          }
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
    action.add.active = false;
    action.add.text[0] = '\0';
    action.add.row = pastePos.row;
    action.add.col = pastePos.col;

    action.delete.active = false;
    action.delete.text[0] = '\0';
    action.delete.row = pastePos.row;
    action.delete.col = pastePos.col;

    // Record removed text (dari selection yg ditimpa) sebagai delete part
    if (insertResult.removed && insertResult.removedLen > 0)
    {
      action.delete.active = true;
      action.delete.row = pastePos.row;
      action.delete.col = pastePos.col;

      int copyLen = insertResult.removedLen;
      if (copyLen > HISTORY_ACTION_BUFFER_SIZE - 1)
        copyLen = HISTORY_ACTION_BUFFER_SIZE - 1;

      if (copyLen > 0)
      {
        memcpy(action.delete.text, insertResult.removed, (size_t)copyLen);
        action.delete.text[copyLen] = '\0';
      }
    }

    // Record inserted text sebagai add part
    if (insertResult.insertedLen > 0)
    {
      action.add.active = true;
      int copyLen = insertResult.insertedLen;
      if (copyLen > HISTORY_ACTION_BUFFER_SIZE - 1)
        copyLen = HISTORY_ACTION_BUFFER_SIZE - 1;

      if (copyLen > 0)
      {
        memcpy(action.add.text, insertResult.inserted, (size_t)copyLen);
        action.add.text[copyLen] = '\0';
      }
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

  case ID_EDIT_FIND:
      Search_ShowFindDialog(hWnd);
      return 0;

  case ID_EDIT_REPLACE:
      Search_ShowReplaceDialog(hWnd);
      return 0;

  case ID_EDIT_GOTO:
      Search_ShowGotoDialog(hWnd);
      return 0;

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