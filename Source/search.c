#include "../Header/search.h"
#include "../Header/selection.h"
#include "../Header/cursor.h"
#include "../Header/goto.h"
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

SearchState g_searchState = {0};

void Search_Init() {
  memset(&g_searchState, 0, sizeof(SearchState));

  // Dapatkan ID unik dari Windows
  g_searchState.uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);

  // Setup FINDREPLACE struct dasar
  g_searchState.fr.lStructSize = sizeof(FINDREPLACE);
  g_searchState.fr.lpstrFindWhat = g_searchState.szFindWhat;
  g_searchState.fr.wFindWhatLen = sizeof(g_searchState.szFindWhat);
  g_searchState.fr.lpstrReplaceWith = g_searchState.szReplaceWith;
  g_searchState.fr.wReplaceWithLen = sizeof(g_searchState.szReplaceWith);
}

void Search_ShowFindDialog(HWND hwndOwner) {
  if (g_searchState.hDlg != NULL) {
    SetFocus(g_searchState.hDlg);
    return;
  }
  g_searchState.fr.hwndOwner = hwndOwner;
  g_searchState.fr.Flags = FR_DOWN | FR_HIDEWHOLEWORD;
  g_searchState.hDlg = FindText(&g_searchState.fr);
}

void Search_ShowReplaceDialog(HWND hwndOwner) {
  if (g_searchState.hDlg != NULL) {
    SetFocus(g_searchState.hDlg);
    return;
  }
  g_searchState.fr.hwndOwner = hwndOwner;
  g_searchState.fr.Flags = FR_DOWN | FR_HIDEWHOLEWORD;
  g_searchState.hDlg = ReplaceText(&g_searchState.fr);
}

void Search_HandleMessage(HWND hWnd, AppState *s, LPARAM lParam) {
  LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;

  // Jika user menutup dialog (klik X)
  if (lpfr->Flags & FR_DIALOGTERM) {
    g_searchState.hDlg = NULL;
    return;
  }

  BOOL matchCase = (lpfr->Flags & FR_MATCHCASE) ? TRUE : FALSE;
  BOOL searchDown = (lpfr->Flags & FR_DOWN) ? TRUE : FALSE;

  if (lpfr->Flags & FR_FINDNEXT) {
    Search_FindNext(hWnd, s, g_searchState.szFindWhat, matchCase, searchDown, FALSE);
  }
  else if (lpfr->Flags & FR_REPLACE)
  {
    Search_ReplaceCurrent(hWnd, s, matchCase, g_searchState.szFindWhat, g_searchState.szReplaceWith);
    Search_FindNext(hWnd, s, g_searchState.szFindWhat, matchCase, searchDown, FALSE);
  }
  else if (lpfr->Flags & FR_REPLACEALL)
  {
    Search_ReplaceAll(hWnd, s, g_searchState.szFindWhat,g_searchState.szReplaceWith, matchCase);
  }
}

BOOL Search_FindNext(HWND hWnd, AppState *s, const char* findWhat, BOOL matchCase, BOOL searchDown, BOOL silent) {
    TextBuffer* buf = &s->textBuffer;
    int findLen = strlen(findWhat);
    if (findLen == 0) return FALSE;

    int startRow = buf->cursorRow;
    int startCol = buf->cursorCol;

    // PASS 1: Cari dari posisi kursor ke akhir dokumen
    if (searchDown) {
      // Search forward: startRow → end of document
      for (int r = startRow; r < Buffer_GetLineCount(buf); r++) {
          int c_start = (r == startRow) ? startCol : 0;
          int lineLen = Buffer_GetLineLen(buf, r);
          if (c_start > lineLen) c_start = lineLen;

          const char *line = Buffer_GetLineText(buf, r);
          const char *found = NULL;
          if (matchCase)
              found = strstr(line + c_start, findWhat);
          else
              found = StrStrI(line + c_start, findWhat);

          if (found) {
              int foundCol = (int)(found - line);
              Selection_SetSelection(s, 1, r, foundCol, r, foundCol + findLen);
              Cursor_SetPosition(buf, r, foundCol + findLen);
              App_RefreshEditorAfterAction(hWnd, s);
              return TRUE;
          }
      }

      // PASS 2: Wrap around — search from line 0 up to startRow
      for (int r = 0; r <= startRow; r++) {
        // On the last row of this pass, stop at the original startCol to avoid re-finding
        int c_end_limit = (r == startRow) ? startCol : Buffer_GetLineLen(buf, r);

        const char *line = Buffer_GetLineText(buf, r);
        const char *found = NULL;
        if (matchCase)
          found = strstr(line, findWhat);
        else
          found = StrStrI(line, findWhat);


        if (found && (int)(found - line) + findLen <= c_end_limit) {
          int foundCol = (int)(found - line);
          Selection_SetSelection(s, 1, r, foundCol, r, foundCol + findLen);
          Cursor_SetPosition(buf, r, foundCol + findLen);
          App_RefreshEditorAfterAction(hWnd, s);
          return TRUE;
        }
      }

    }
    else {
      // Search backward: startRow → beginning of document
      // We search each line from right-to-left by scanning backward manually.
      for (int r = startRow; r >= 0; r--) {
          int lineLen = Buffer_GetLineLen(buf, r);
          // On the start row, we must only look at characters BEFORE startCol
          int c_end = (r == startRow) ? startCol : lineLen;

          // Scan backwards within the line for the last match before c_end
          const char *lastFound = NULL;
          const char *search_ptr = Buffer_GetLineText(buf, r);
          while (1) {
              const char *candidate =  NULL;
                if (matchCase)
                  candidate = strstr(search_ptr, findWhat);
                else
                  candidate = StrStrI(search_ptr, findWhat);

              if (!candidate) break;
              if ((int)(candidate - Buffer_GetLineText(buf, r)) + findLen > c_end) break;
              lastFound = candidate;
              search_ptr = candidate + 1; // advance past this match
          }

          if (lastFound) {
              int foundCol = (int)(lastFound - Buffer_GetLineText(buf, r));
              Selection_SetSelection(s, 1, r, foundCol, r, foundCol + findLen);
              Cursor_SetPosition(buf, r, foundCol);  // cursor at START for Find Prev
              App_RefreshEditorAfterAction(hWnd, s);
              return TRUE;
          }
      }

      //Wrap Around
      for (int r = Buffer_GetLineCount(buf) - 1; r >= startRow; r--) {
        // Scan backwards within the line for the last match after c_end
        const char *lastFound = NULL;
        const char *search_ptr = (r == startRow) ? Buffer_GetLineText(buf, r) + startCol + findLen : Buffer_GetLineText(buf, r);
        while (1) {
          const char *candidate =  NULL;
          if (matchCase)
            candidate = strstr(search_ptr, findWhat);
          else
            candidate = StrStrI(search_ptr, findWhat);

          if (!candidate) break;
          lastFound = candidate;
          search_ptr = candidate + 1; // advance past this match
        }

        if (lastFound) {
          int foundCol = (int)(lastFound - Buffer_GetLineText(buf, r));
          Selection_SetSelection(s, 1, r, foundCol, r, foundCol + findLen);
          Cursor_SetPosition(buf, r, foundCol);  // cursor at START for Find Prev
          App_RefreshEditorAfterAction(hWnd, s);
          return TRUE;
        }
      }
    }

  if (!silent)
    MessageBox(g_searchState.hDlg ? g_searchState.hDlg : NULL, "Teks tidak ditemukan.", "Archivista", MB_OK | MB_ICONINFORMATION);
    return FALSE;
}


void Search_ReplaceCurrent(HWND hWnd, AppState *s, BOOL matchCase, const char* findWhat, const char* replaceWith) {
  TextBuffer *buf = &s->textBuffer;

  if (!s->selection.active)
    return;

  char *selected = Buffer_GetSelectedString(buf, &s->selection);

  if (matchCase ? strcmp(selected, findWhat) != 0 : stricmp(selected, findWhat) != 0) {
    free(selected);
    return;
  }

  free(selected);
  InsertStringResult result = Buffer_InsertString(buf, replaceWith, &s->selection);
  Buffer_FreeInsertStringResult(&result);

  Selection_SetSelection(s, 0, buf->cursorRow, buf->cursorCol, buf->cursorRow, buf->cursorCol);

  s->isEdited = TRUE;
  App_RefreshEditorAfterAction(hWnd, s);
}

void Search_ReplaceAll(HWND hWnd, AppState *s,
  const char* findWhat, const char* replaceWith, BOOL matchCase) {
  Buffer_SetCursorPosition(&s->textBuffer, 0, 0);
  s->selection.active = 0;
  int counter = 0;
  int prevRow = -1, prevCol = -1;
  while (Search_FindNext(hWnd, s, findWhat, matchCase, TRUE, TRUE)) {
    int curRow = s->textBuffer.cursorRow;
    int curCol = s->textBuffer.cursorCol;
    // Guard: jika kursor tidak maju setelah replace, hentikan untuk cegah infinite loop
    if (curRow == prevRow && curCol == prevCol)
      break;
    prevRow = curRow;
    prevCol = curCol;
    Search_ReplaceCurrent(hWnd, s, matchCase,findWhat , replaceWith);
    counter++;
  }
  char msg[64];
  sprintf(msg, "Berhasil mengganti %d kata.", counter);
  MessageBox(NULL, msg, "Replace All", MB_OK | MB_ICONINFORMATION);
}

// Function to initialize and display the custom dialog
void Search_ShowGotoDialog(HWND hwndOwner) {
    AppState *appState = App_GetState(hwndOwner);
    if (!appState) return;
    static BOOL registered = FALSE;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwndOwner, GWLP_HINSTANCE);
    if (!registered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = GotoDlgProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wc.lpszClassName = "ArchivistaGotoLine";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&wc);
        registered = TRUE;
    }
    GotoDlgContext *ctx = (GotoDlgContext *)calloc(1, sizeof(GotoDlgContext));
    ctx->hwndOwner = hwndOwner;
    ctx->appState = appState;
    // Center dialog box relative to the owner window
    RECT parentRect;
    GetWindowRect(hwndOwner, &parentRect);
    int width = 295;
    int height = 150;
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
    HWND hwndDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "ArchivistaGotoLine",
        "Go To Line",
        WS_POPUPWINDOW | WS_CAPTION,
        x, y, width, height,
        hwndOwner,
        NULL,
        hInstance,
        ctx
    );
    if (hwndDlg) {
        EnableWindow(hwndOwner, FALSE); // Disable main editor interactions (making dialog modal)
        ShowWindow(hwndDlg, SW_SHOW);
        UpdateWindow(hwndDlg);
    } else {
        free(ctx);
    }
}