#ifndef ARCHIVISTA_RECENT_H
#define ARCHIVISTA_RECENT_H

#include "app.h"

// Membaca history dari recent.ini ke AppState saat aplikasi dimulai
void Recent_LoadRecent(AppState *s);

// Menyimpan file baru ke urutan pertama, lalu menggeser yang lama
void Recent_AddRecent(AppState *s, const char* filepath);

// Memperbarui UI Menu "File" agar menampilkan list file history
void Recent_UpdateMenuRecent(HMENU hMenuBar, AppState *s);

// Getter untuk mendapatkan path recent file dengan index
char *Recent_GetRecentPathByIndex(int index);


#endif //ARCHIVISTA_RECENT_H