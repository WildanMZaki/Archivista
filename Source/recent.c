#include "../Header/recent.h"
#include "../Header/menu.h"
#include "stdio.h"

static RecentFiles recents;

static void NRLL_Initialize(RecentFiles *recent) {
  recent->First = NULL;
  recent->recentFileCount = 0;
}

static BOOL NRLL_IsEmpty(RecentFiles *recent) {
  return recent->First == NULL;
}

static RecentFileNode *NRLL_Allocation(const char *path) {
  RecentFileNode *node = (RecentFileNode *)malloc(sizeof(RecentFileNode));
  if (node != NULL) {
    strcpy(node->pathFiles, path);
    node->next = NULL;
    node->prev = NULL;
  }
  return node;
}

static void NRLL_DeAllocation(RecentFileNode *node) { free(node); }

static RecentFileNode *NRLL_GetLastNode(RecentFiles *recent) {
  RecentFileNode *last = recent->First;
  if (last != NULL) {
    while (last->next != NULL)
      last = last->next;
    return last;
  }

  return NULL;
}

static RecentFileNode *NRLL_FindNodeByPath(RecentFiles *recent,
                                           const char *path) {
  RecentFileNode *node = recent->First;
  while (node != NULL) {
    if (strcmp(node->pathFiles, path) == 0) {
      return node;
    }
    node = node->next;
  }

  return NULL; // ret NULL if not found
}

static void NRLL_InsertFirst(RecentFiles *recent, const char *path) {
  RecentFileNode *newNode = NRLL_Allocation(path);
  if (newNode == NULL)
    return;

  if (NRLL_IsEmpty(recent)) {
    recent->First = newNode;
  } else {
    recent->First->prev = newNode;
    newNode->next = recent->First;
    recent->First = newNode;
  }

  recent->recentFileCount++;
}

static void NRLL_InsertLast(RecentFiles *recent, const char *path) {
  RecentFileNode *newNode = NRLL_Allocation(path);

  if (newNode == NULL)
    return;

  if (NRLL_IsEmpty(recent)) {
    recent->First = newNode;
  } else {
    RecentFileNode *last = NRLL_GetLastNode(recent);
    last->next = newNode;
    newNode->prev = last;
  }

  recent->recentFileCount++;
}

static void NRLL_DeleteNode(RecentFiles *recent, RecentFileNode *node) {
  if (node->prev != NULL)
    node->prev->next = node->next;
  else
    recent->First = node->next; // untuk memindahkan first ke next node jika
                                // yang dihapus adalah first node.
  if (node->next != NULL) {
    node->next->prev = node->prev;
  }

  NRLL_DeAllocation(node);
  recent->recentFileCount--;
}

static void NRLL_DeleteLast(RecentFiles *recent) {
  RecentFileNode *last = NRLL_GetLastNode(recent);
  if (last != NULL)
    NRLL_DeleteNode(recent, last);
}

void Recent_FreeAllNode() {
  RecentFileNode *curr = recents.First;
  while (curr != NULL) {
    RecentFileNode *next = curr->next;
    NRLL_DeleteNode(&recents, curr);
    curr = next;
  }
  recents.First = NULL;
}

// Fungsi bantuan untuk mendapatkan Full Path lokasi recent.ini
static void GetIniPath(char *outPath) {
  GetModuleFileName(NULL, outPath, MAX_PATH);
  char *lastSlash = strrchr(outPath, '\\');
  if (lastSlash) {
    strcpy(lastSlash + 1, RECENT_INI_NAME);
  } else {
    strcpy(outPath, RECENT_INI_NAME);
  }
}

void Recent_LoadRecent() {
  char iniPath[MAX_PATH];
  GetIniPath(iniPath);

  NRLL_Initialize(&recents);
  for (int i = 0; i < MAX_RECENT_FILES; i++) {
    char keyName[16];
    sprintf(keyName, "File%d", i + 1);

    // Membaca nilai dari recent.ini menggunakan API Bawaan Windows
    char buffer[MAX_PATH];
    GetPrivateProfileString("RecentFiles", keyName, "", buffer, MAX_PATH,
                            iniPath);

    if (strlen(buffer) > 0) {
      NRLL_InsertLast(&recents, buffer);
    }
  }
}

void Recent_AddRecent(const char *filepath) {
  RecentFileNode *node = NRLL_FindNodeByPath(&recents, filepath);
  if (node != NULL) {
    NRLL_DeleteNode(&recents, node);
  }

  if (recents.recentFileCount >= MAX_RECENT_FILES) {
    NRLL_DeleteLast(&recents);
  }

  // Taruh posisi index ke-0
  NRLL_InsertFirst(&recents, filepath);

  // Simpan (Save) data terbaru ke recent.ini
  char iniPath[MAX_PATH];
  GetIniPath(iniPath);
  node = recents.First;
  int i = 1;
  while (node != NULL && i <= MAX_RECENT_FILES) {
    char keyName[6];
    sprintf(keyName, "File%d", i);
    WritePrivateProfileString("RecentFiles", keyName, node->pathFiles, iniPath);
    node = node->next;
    i++;
  };
}

void Recent_UpdateMenuRecent(HMENU hMenuBar) {
  // Kita ambil submenu 'File' (index ke-0 dari MenuBar)
  HMENU hFileMenu = GetSubMenu(hMenuBar, 0);
  if (!hFileMenu)
    return;

  // Hapus history lama kalau ada (untuk menghindari menu menumpuk saat di
  // refresh)
  for (int i = 0; i < MAX_RECENT_FILES; i++) {
    DeleteMenu(hFileMenu, ID_FILE_RECENT_START + i, MF_BYCOMMAND);
  }

  // Hapus Separator penanda history kalau sudah ada
  DeleteMenu(hFileMenu, ID_FILE_RECENT_SEPARATOR, MF_BYCOMMAND);

  // Tambahkan kembali history yang terupdate
  if (!NRLL_IsEmpty(&recents)) {
    AppendMenu(hFileMenu, MF_SEPARATOR, ID_FILE_RECENT_SEPARATOR, NULL);
    RecentFileNode *node = recents.First;
    int i = 0;
    while (node != NULL && i < MAX_RECENT_FILES) {
      char menuText[MAX_PATH + 2];
      sprintf(menuText, "%d %s", i + 1,
              node->pathFiles); // Format: "&1 C:\..."
      AppendMenu(hFileMenu, MF_STRING, ID_FILE_RECENT_START + i, menuText);
      node = node->next;
      i++;
    }
  }
}

char *Recent_GetRecentPathByIndex(int index) {
  if (index < recents.recentFileCount) {
    RecentFileNode *node = recents.First;
    for (int i = 0; i < index; i++) {
      node = node->next;
    }
    return node->pathFiles;
  }
  return NULL;
}
