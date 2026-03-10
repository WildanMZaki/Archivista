#ifndef ARCHIVISTA_BUFFER_H
#define ARCHIVISTA_BUFFER_H

#define INITIAL_LINE_CAPACITY 16   // Initial capacity for characters in a line
#define INITIAL_BUFFER_CAPACITY 16 // Initial capacity for number of lines

// ========== Structs ==========

typedef struct
{
    char *text;   // Array of characters (isi satu baris)
    int length;   // Jumlah karakter aktual
    int capacity; // Kapasitas allocated
} Line;

typedef struct
{
    Line *lines;      // Array of Line (array 2D)
    int lineCount;    // Jumlah baris aktual
    int lineCapacity; // Kapasitas allocated untuk lines
    int cursorRow;    // Posisi cursor - baris
    int cursorCol;    // Posisi cursor - kolom
} TextBuffer;

// ========== Lifecycle ==========
void Buffer_Init(TextBuffer *buf);
void Buffer_Free(TextBuffer *buf);
void Buffer_Clear(TextBuffer *buf);

// ========== Line Operations (internal helpers) ==========
void Line_Init(Line *line);
void Line_Free(Line *line);
void Line_EnsureCapacity(Line *line, int needed);

// ========== Text Editing ==========
void Buffer_InsertChar(TextBuffer *buf, char c);
void Buffer_InsertNewline(TextBuffer *buf);
void Buffer_Backspace(TextBuffer *buf);
void Buffer_Delete(TextBuffer *buf);

// ========== Conversion ==========
char *Buffer_ToString(TextBuffer *buf); // Caller must free() the result
void Buffer_FromString(TextBuffer *buf, const char *str);

#endif // ARCHIVISTA_BUFFER_H