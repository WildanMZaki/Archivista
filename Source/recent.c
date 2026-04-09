#include "../Header/recent.h"
#include "../Header/menu.h"
#include "stdio.h"

// Fungsi bantuan untuk mendapatkan Full Path lokasi recent.ini
static void GetIniPath(char* outPath) {
    GetModuleFileName(NULL, outPath, MAX_PATH);
    char *lastSlash = strrchr(outPath, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, RECENT_INI_NAME);
    } else {
        strcpy(outPath, RECENT_INI_NAME);
    }
}

void Recent_LoadRecent(AppState *s) {
    char iniPath[MAX_PATH];
    GetIniPath(iniPath);

    s->recentFileCount = 0;
    for (int i = 0; i < MAX_RECENT_FILES; i++) {
        char keyName[16];
        sprintf(keyName, "File%d", i + 1);

        // Membaca nilai dari recent.ini menggunakan API Bawaan Windows
        char buffer[MAX_PATH];
        GetPrivateProfileString("RecentFiles", keyName, "", buffer, MAX_PATH, iniPath);

        if (strlen(buffer) > 0) {
            strcpy(s->recentFiles[i], buffer);
            s->recentFileCount++;
        } else {
            // Jika kosong, kita kosongkan saja stringnya
            s->recentFiles[i][0] = '\0';
        }
    }
}

void Recent_AddRecent(AppState *s, const char* filepath) {
    // 1. Cek apakah file sudah ada di history
    int existingIndex = -1;
    for (int i = 0; i < s->recentFileCount; i++) {
        if (strcmp(s->recentFiles[i], filepath) == 0) {
            existingIndex = i;
            break;
        }
    }
    // 2. Jika sudah ada, hapus file lama untuk dipindah ke nomor 1
    if (existingIndex != -1) {
        for (int i = existingIndex; i > 0; i--) {
            strcpy(s->recentFiles[i], s->recentFiles[i - 1]);
        }
    }
    // 3. Jika belum ada, geser semua ke bawah (sampai batas maks)
    else {
        int shiftCount = s->recentFileCount < MAX_RECENT_FILES ? s->recentFileCount : MAX_RECENT_FILES - 1;
        for (int i = shiftCount; i > 0; i--) {
            strcpy(s->recentFiles[i], s->recentFiles[i - 1]);
        }
        if (s->recentFileCount < MAX_RECENT_FILES) {
            s->recentFileCount++;
        }
    }

    // 4. Taruh posisi index ke-0
    strcpy(s->recentFiles[0], filepath);
    // 5. Simpan (Save) data terbaru ke recent.ini
    char iniPath[MAX_PATH];
    GetIniPath(iniPath);
    for (int i = 0; i < s->recentFileCount; i++) {
        char keyName[16];
        sprintf(keyName, "File%d", i + 1);
        WritePrivateProfileString("RecentFiles", keyName, s->recentFiles[i], iniPath);
    }
}

void Recent_UpdateMenuRecent(HMENU hMenuBar, AppState *s) {
    // Kita ambil submenu 'File' (index ke-0 dari MenuBar)
    HMENU hFileMenu = GetSubMenu(hMenuBar, 0);
    if (!hFileMenu) return;

    // Hapus history lama kalau ada (untuk menghindari menu menumpuk saat di refresh)
    for (int i = 0; i < MAX_RECENT_FILES; i++) {
        DeleteMenu(hFileMenu, ID_FILE_RECENT_START + i, MF_BYCOMMAND);
    }

    // Hapus Separator penanda history kalau sudah ada
    DeleteMenu(hFileMenu, 1100, MF_BYCOMMAND);

    // Tambahkan kembali history yang terupdate
    if (s->recentFileCount > 0) {
        AppendMenu(hFileMenu, MF_SEPARATOR, 1100, NULL);
        for (int i = 0; i < s->recentFileCount; i++) {
            char menuText[MAX_PATH + 3];
            sprintf(menuText, "&%d %s", i + 1, s->recentFiles[i]); // Format: "&1 C:\..."
            AppendMenu(hFileMenu, MF_STRING, ID_FILE_RECENT_START + i, menuText);
        }
    }
}