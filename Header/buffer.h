#ifndef ARCHIVISTA_BUFFER_H
#define ARCHIVISTA_BUFFER_H

// Static 2D buffer limits (ubah sesuai requirement dosen)
#define BUF_MAX_LINES 2000
#define BUF_MAX_COLS 512 // termasuk '\0'

typedef struct
{
    // lines[row] adalah string null-terminated
    char lines[BUF_MAX_LINES][BUF_MAX_COLS];
    int lineLen[BUF_MAX_LINES]; // panjang aktual tiap baris (tanpa '\0')

    int lineCount; // minimal 1
    int cursorRow;
    int cursorCol;
} TextBuffer;

typedef struct
{
    int row;
    int col;
} TextPos;

typedef struct
{
    int active;
    TextPos start;
    TextPos end;
} TextSelection;

// ========== Lifecycle ==========
void Buffer_Init(TextBuffer *buf);
void Buffer_Free(TextBuffer *buf);  // untuk static: reset/no-op
void Buffer_Clear(TextBuffer *buf); // reset jadi 1 baris kosong

// ========== Text Editing ==========
void Buffer_InsertChar(TextBuffer *buf, char c);
void Buffer_InsertNewline(TextBuffer *buf);
void Buffer_Backspace(TextBuffer *buf);
void Buffer_Delete(TextBuffer *buf);

// ========== Selection ==========
int Buffer_HasSelection(const TextBuffer *buf, const TextSelection *sel);
void Buffer_NormalizeSelection(const TextBuffer *buf, const TextSelection *sel, TextPos *outStart, TextPos *outEnd);
char *Buffer_GetSelectedString(const TextBuffer *buf, const TextSelection *sel); // Caller must free() result
int Buffer_DeleteSelection(TextBuffer *buf, const TextSelection *sel);           // Return 1 if deleted

// ========== Conversion ==========
char *Buffer_ToString(TextBuffer *buf); // Caller must free() the result
void Buffer_FromString(TextBuffer *buf, const char *str);

#endif // ARCHIVISTA_BUFFER_H