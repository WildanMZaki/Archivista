#ifndef ARCHIVISTA_BUFFER_H
#define ARCHIVISTA_BUFFER_H

// Max visible chars per row = 1000, plus 1 for '\0'
#define BUF_MAX_COLS 1001
// #define BUF_MAX_COLS 11

typedef struct TextLineNode
{
    char text[BUF_MAX_COLS];
    int len;
    int isWrapped; // 1 if this node was created by automatic overflow (soft-wrap), 0 for explicit newline
    struct TextLineNode *prev;
    struct TextLineNode *next;
} TextLineNode;

typedef struct
{
    TextLineNode *head;
    TextLineNode *tail;
    TextLineNode *cursorNode;
    char *initSnapshot;

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

typedef struct
{
    char *removed;  // String yang dihapus dari selection (caller must free)
    char *inserted; // String yang benar-benar diinsert, setelah handle newline & truncate (caller must free)
    int removedLen;
    int insertedLen;
} InsertStringResult;

// ========== Lifecycle ==========
void Buffer_Init(TextBuffer *buf);
void Buffer_Free(TextBuffer *buf);  // untuk static: reset/no-op
void Buffer_Clear(TextBuffer *buf); // reset jadi 1 baris kosong
void Buffer_SetInitBuffer(TextBuffer *buf);
int Buffer_IsBufferChanged(const TextBuffer *buf);
int Buffer_IsBufferSavable(const TextBuffer *buf);
int Buffer_GetLineCount(const TextBuffer *buf);
const char *Buffer_GetLineText(const TextBuffer *buf, int row);
int Buffer_GetLineLen(const TextBuffer *buf, int row);
void Buffer_SetCursorPosition(TextBuffer *buf, int row, int col);

// ========== Text Editing ==========
void Buffer_InsertChar(TextBuffer *buf, char c);
void Buffer_InsertNewline(TextBuffer *buf);
void Buffer_Backspace(TextBuffer *buf);
void Buffer_Delete(TextBuffer *buf);
InsertStringResult Buffer_InsertString(TextBuffer *buf, const char *str, const TextSelection *sel);
void Buffer_FreeInsertStringResult(InsertStringResult *result);

// ========== Selection ==========
int Buffer_HasSelection(const TextBuffer *buf, const TextSelection *sel);
void Buffer_NormalizeSelection(const TextBuffer *buf, const TextSelection *sel, TextPos *outStart, TextPos *outEnd);
char *Buffer_GetSelectedString(const TextBuffer *buf, const TextSelection *sel); // Caller must free() result
int Buffer_DeleteSelection(TextBuffer *buf, const TextSelection *sel);           // Return 1 if deleted

// ========== Conversion ==========
char *Buffer_ToString(const TextBuffer *buf); // Caller must free() the result
void Buffer_FromString(TextBuffer *buf, const char *str);

#endif // ARCHIVISTA_BUFFER_H