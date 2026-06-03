#ifndef ARCHIVISTA_SEARCH_H
#define ARCHIVISTA_SEARCH_H

#include "app.h"
#include "history.h"
#include <commdlg.h>
#include <windows.h>


// State untuk menyimpan konfigurasi Find & Replace
typedef struct {
  HWND hDlg;               // Handle dialog Find/Replace yang sedang aktif
  UINT uFindReplaceMsg;    // ID Message khusus dari Windows
  FINDREPLACE fr;          // Struct bawaan Win32
  char szFindWhat[256];    // Buffer teks yang dicari
  char szReplaceWith[256]; // Buffer teks pengganti
} SearchState;

// DEKLARASI GLOBAL (Agar bisa dipakai di main.c dan winproc.c)
extern SearchState g_searchState;

// Inisialisasi
void Search_Init();

// UI Dialog
void Search_ShowFindDialog(HWND hwndOwner);
void Search_ShowReplaceDialog(HWND hwndOwner);
void Search_ShowGotoDialog(HWND hwndOwner);

// Logika Message
void Search_HandleMessage(HWND hWnd, AppState* appState, LPARAM lParam);

// Logika Pencarian & Replace Internal
BOOL Search_FindNext(HWND hWnd, AppState *s, const char* findWhat, BOOL matchCase, BOOL searchDown, BOOL silent);
void Search_ReplaceCurrent(HWND hWnd, AppState *s, BOOL matchCase, const char* findWhat, const char* replaceWith, HistoryAction *groupAction);
void Search_ReplaceAll(HWND hWnd, AppState* appState, const char* findWhat, const char* replaceWith, BOOL matchCase);

#endif // ARCHIVISTA_SEARCH_H