#ifndef ARCHIVISTA_RECENT_H
#define ARCHIVISTA_RECENT_H

#include "app.h"

#define MAX_RECENT_FILES 5
#define RECENT_INI_NAME "recent.ini"
#define ID_FILE_RECENT_SEPARATOR 1100

typedef struct tRecentFile {
    char pathFiles[MAX_PATH];
    struct tRecentFile *next;
    struct tRecentFile *prev;
} RecentFileNode;

typedef struct {
    RecentFileNode *First;
    int recentFileCount;
} RecentFiles;

// Membaca history dari recent.ini ke AppState saat aplikasi dimulai
void Recent_LoadRecent();

// Menyimpan file baru ke urutan pertama, lalu menggeser yang lama
void Recent_AddRecent(const char* filepath);

// Memperbarui UI Menu "File" agar menampilkan list file history
void Recent_UpdateMenuRecent(HMENU hMenuBar);

// Getter untuk mendapatkan path recent file dengan index
char *Recent_GetRecentPathByIndex(int index);

// Untuk Free Semua Node
void Recent_FreeAllNode();

#endif //ARCHIVISTA_RECENT_H