#include "../Header/fileops.h"
#include "../Header/buffer.h"

static OPENFILENAME InitOpenFile(HWND hWnd, AppState *s) {
  OPENFILENAME ofn = {sizeof(OPENFILENAME)};
  ofn.hwndOwner = hWnd;
  ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = s->currentFilePath;
  ofn.lpstrDefExt = "txt";
  ofn.nMaxFile = sizeof(s->currentFilePath);
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  return ofn;
}

static void ResetCursor(HWND hWnd, AppState *s) {
  s->selection.active = 0;           // Disable Selection
  s->scrollX = 0;                    // Reset Scroll X
  s->scrollY = 0;                    // Reset Scroll Y
  InvalidateRect(hWnd, NULL, FALSE); // Refresh Screen
}

void FileOps_New(HWND hWnd, AppState *s) {
  Buffer_Clear(&s->textBuffer); // Clear Buffer
  s->currentFilePath[0] = '\0'; // Reset Current File Path
  ResetCursor(hWnd, s);
}

static void FileOps_OpenFile(HWND hWnd, AppState *s, char *path) {
  HANDLE hFile = CreateFile(path, GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    MessageBox(hWnd, "Failed to open file", "Error", MB_ICONERROR);
    return;
  }
  DWORD fileSize = GetFileSize(hFile, NULL);
  if (fileSize == INVALID_FILE_SIZE) {
    MessageBox(hWnd, "Failed to get file size", "Error", MB_ICONERROR);
    CloseHandle(hFile);
    return;
  }
  char *buffer = (char *)malloc(fileSize + 1);
  if (!buffer) {
    MessageBox(hWnd, "Failed to allocate memory", "Error", MB_ICONERROR);
    CloseHandle(hFile);
    return;
  }
  DWORD bytesRead;
  if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
    MessageBox(hWnd, "Failed to read file", "Error", MB_ICONERROR);
    free(buffer);
    CloseHandle(hFile);
    return;
  }
  CloseHandle(hFile);
  Buffer_Clear(&s->textBuffer);
  Buffer_FromString(&s->textBuffer, buffer);
  free(buffer);
}

void FileOps_Open(HWND hWnd, AppState *s, char *path) {
    // Initialize Open File Dialog
    if (path != NULL) {
      FileOps_OpenFile(hWnd, s, path);
    }
    else {
      OPENFILENAME ofn = InitOpenFile(hWnd, s);
      ofn.lpstrTitle = "Open File";

      if (GetOpenFileName(&ofn)) {
        FileOps_OpenFile(hWnd, s, ofn.lpstrFile);
      }else {
        MessageBox(hWnd, "Failed to open file", "Error", MB_ICONERROR);
      }
    }
    ResetCursor(hWnd, s);
  }
}

void FileOps_Save(HWND hWnd, AppState *s) {
  if (s->currentFilePath[0] == '\0') {
    FileOps_SaveAs(hWnd, s);
    return;
  }
  HANDLE hFile = CreateFile(s->currentFilePath, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    MessageBox(hWnd, "Failed to save file", "Error", MB_ICONERROR);
    return;
  }
  DWORD bytesWritten;
  char *strBuf = Buffer_ToString(&s->textBuffer);
  if (!WriteFile(hFile, strBuf, strlen(strBuf), &bytesWritten, NULL)) {
    MessageBox(hWnd, "Failed to write file", "Error", MB_ICONERROR);
    free(strBuf);
    CloseHandle(hFile);
    return;
  }
  Recent_AddRecent(s, s->currentFilePath);
  Recent_UpdateMenuRecent(GetMenu(hWnd), s);
  free(strBuf);
  CloseHandle(hFile);
}

void FileOps_SaveAs(HWND hWnd, AppState *s) {
  // Initialize Save As Dialog
  OPENFILENAME ofn = InitOpenFile(hWnd, s);
  ofn.lpstrTitle = "Save As";

  if (GetSaveFileName(&ofn)) {
    HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      MessageBox(hWnd, "Failed to save file", "Error", MB_ICONERROR);
      return;
    }
    DWORD bytesWritten;
    char *strBuf = Buffer_ToString(&s->textBuffer);
    if (!WriteFile(hFile, strBuf, strlen(strBuf), &bytesWritten, NULL)) {
      MessageBox(hWnd, "Failed to write file", "Error", MB_ICONERROR);
      free(strBuf);
      CloseHandle(hFile);
      return;
    }
    Recent_AddRecent(s, ofn.lpstrFile);
    Recent_UpdateMenuRecent(GetMenu(hWnd), s);
    free(strBuf);
    CloseHandle(hFile);
  }
}