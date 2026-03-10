#include "../Header/buffer.h"
#include <stdlib.h>
#include <string.h>

// =============================================================
//  Line Operations
// =============================================================

void Line_Init(Line *line)
{
    line->capacity = INITIAL_LINE_CAPACITY;
    line->text = (char *)malloc(line->capacity * sizeof(char));
    line->text[0] = '\0';
    line->length = 0;
}

void Line_Free(Line *line)
{
    if (line->text)
    {
        free(line->text);
        line->text = NULL;
    }
    line->length = 0;
    line->capacity = 0;
}

void Line_EnsureCapacity(Line *line, int needed)
{
    if (needed + 1 > line->capacity)
    { // +1 for null terminator
        while (line->capacity < needed + 1)
        {
            line->capacity *= 2;
        }
        line->text = (char *)realloc(line->text, line->capacity * sizeof(char));
    }
}

// =============================================================
//  Buffer Lifecycle
// =============================================================

// Pastikan buffer punya minimal 1 line kosong (siap pakai)
void Buffer_Init(TextBuffer *buf)
{
    buf->lineCapacity = INITIAL_BUFFER_CAPACITY;
    buf->lines = (Line *)malloc(buf->lineCapacity * sizeof(Line));
    buf->lineCount = 1;
    buf->cursorRow = 0;
    buf->cursorCol = 0;

    Line_Init(&buf->lines[0]); // Mulai dengan 1 baris kosong
}

// Bebaskan SEMUA memori
void Buffer_Free(TextBuffer *buf)
{
    for (int i = 0; i < buf->lineCount; i++)
    {
        Line_Free(&buf->lines[i]);
    }
    free(buf->lines);
    buf->lines = NULL;
    buf->lineCount = 0;
    buf->lineCapacity = 0;
    buf->cursorRow = 0;
    buf->cursorCol = 0;
}

// Reset ke kondisi awal (1 baris kosong), tanpa full free
void Buffer_Clear(TextBuffer *buf)
{
    for (int i = 0; i < buf->lineCount; i++)
    {
        Line_Free(&buf->lines[i]);
    }
    buf->lineCount = 1;
    Line_Init(&buf->lines[0]);
    buf->cursorRow = 0;
    buf->cursorCol = 0;
}

// =============================================================
//  Internal Helper: Ensure lines array capacity
// =============================================================

static void Buffer_EnsureLineCapacity(TextBuffer *buf, int needed)
{
    if (needed > buf->lineCapacity)
    {
        while (buf->lineCapacity < needed)
        {
            buf->lineCapacity *= 2;
        }
        buf->lines = (Line *)realloc(buf->lines, buf->lineCapacity * sizeof(Line));
    }
}

// =============================================================
//  Text Editing
// =============================================================

/*
 * Insert satu karakter di posisi cursor.
 *
 * Misal baris sekarang: "Helo World"
 *                cursor col: 3 (^ sebelum 'o')
 *
 * InsertChar(buf, 'l'):
 *   1. Pastikan capacity cukup
 *   2. Geser "o World" ke kanan 1 posisi
 *   3. Taruh 'l' di index 3
 *   4. Hasil: "Hello World"
 *              cursor col: 4
 */
void Buffer_InsertChar(TextBuffer *buf, char c)
{
    Line *line = &buf->lines[buf->cursorRow];

    Line_EnsureCapacity(line, line->length + 1);

    // Geser karakter dari cursorCol ke kanan
    memmove(&line->text[buf->cursorCol + 1],
            &line->text[buf->cursorCol],
            line->length - buf->cursorCol);

    line->text[buf->cursorCol] = c;
    line->length++;
    line->text[line->length] = '\0';
    buf->cursorCol++;
}

/*
 * Handle tombol Enter — pecah baris di posisi cursor.
 *
 * Misal baris 2: "Hello World"
 *          cursor col: 5 (setelah 'o')
 *
 * InsertNewline(buf):
 *   1. Baris 2 jadi: "Hello"
 *   2. Baris baru (3): " World"
 *   3. Semua baris setelahnya geser ke bawah
 *   4. cursor pindah ke row+1, col 0
 */
void Buffer_InsertNewline(TextBuffer *buf)
{
    Line *currentLine = &buf->lines[buf->cursorRow];

    // Sisa teks setelah cursor → jadi baris baru
    int remainLen = currentLine->length - buf->cursorCol;

    // Pastikan array lines cukup
    Buffer_EnsureLineCapacity(buf, buf->lineCount + 1);
    // Re-assign pointer karena realloc mungkin pindah alamat
    currentLine = &buf->lines[buf->cursorRow];

    // Geser semua baris di bawah cursor ke bawah 1 slot
    memmove(&buf->lines[buf->cursorRow + 2],
            &buf->lines[buf->cursorRow + 1],
            (buf->lineCount - buf->cursorRow - 1) * sizeof(Line));

    // Init baris baru
    Line *newLine = &buf->lines[buf->cursorRow + 1];
    Line_Init(newLine);

    // Copy sisa teks ke baris baru
    if (remainLen > 0)
    {
        Line_EnsureCapacity(newLine, remainLen);
        memcpy(newLine->text, &currentLine->text[buf->cursorCol], remainLen);
        newLine->length = remainLen;
        newLine->text[newLine->length] = '\0';
    }

    // Potong baris saat ini di posisi cursor
    currentLine->length = buf->cursorCol;
    currentLine->text[currentLine->length] = '\0';

    buf->lineCount++;
    buf->cursorRow++;
    buf->cursorCol = 0;
}

/*
 * Handle tombol Backspace.
 *
 * Kasus 1 — cursor di tengah baris (col > 0):
 *   "Hello World"  col=5
 *   → Geser " World" ke kiri, hapus 'o'
 *   → "Hell World"  col=4
 *
 * Kasus 2 — cursor di awal baris (col == 0, row > 0):
 *   Baris 1: "Hello"
 *   Baris 2: " World"   ← cursor di sini, col=0
 *   → Merge: baris 1 jadi "Hello World"
 *   → Hapus baris 2, geser baris di bawahnya naik
 *   → cursor pindah ke row=1, col=5 (akhir "Hello")
 */
void Buffer_Backspace(TextBuffer *buf)
{
    if (buf->cursorCol > 0)
    {
        // Kasus 1: Hapus karakter sebelum cursor
        Line *line = &buf->lines[buf->cursorRow];

        memmove(&line->text[buf->cursorCol - 1],
                &line->text[buf->cursorCol],
                line->length - buf->cursorCol);

        line->length--;
        line->text[line->length] = '\0';
        buf->cursorCol--;
    }
    else if (buf->cursorRow > 0)
    {
        // Kasus 2: Merge dengan baris sebelumnya
        Line *prevLine = &buf->lines[buf->cursorRow - 1];
        Line *currLine = &buf->lines[buf->cursorRow];

        int newCursorCol = prevLine->length; // Cursor pindah ke akhir baris sebelumnya

        // Append isi baris saat ini ke baris sebelumnya
        Line_EnsureCapacity(prevLine, prevLine->length + currLine->length);
        memcpy(&prevLine->text[prevLine->length], currLine->text, currLine->length);
        prevLine->length += currLine->length;
        prevLine->text[prevLine->length] = '\0';

        // Free baris saat ini
        Line_Free(currLine);

        // Geser baris di bawahnya naik
        memmove(&buf->lines[buf->cursorRow],
                &buf->lines[buf->cursorRow + 1],
                (buf->lineCount - buf->cursorRow - 1) * sizeof(Line));

        buf->lineCount--;
        buf->cursorRow--;
        buf->cursorCol = newCursorCol;
    }
    // Kalau cursorRow == 0 && cursorCol == 0 → gak ngapa-ngapain
}

/*
 * Handle tombol Delete.
 *
 * Kasus 1 — cursor di tengah baris (col < length):
 *   Sama kayak Backspace tapi hapus karakter DI posisi cursor (bukan sebelumnya)
 *
 * Kasus 2 — cursor di akhir baris (col == length, masih ada baris di bawah):
 *   Merge baris saat ini dengan baris di bawahnya
 */
void Buffer_Delete(TextBuffer *buf)
{
    Line *line = &buf->lines[buf->cursorRow];

    if (buf->cursorCol < line->length)
    {
        // Kasus 1: Hapus karakter di posisi cursor
        memmove(&line->text[buf->cursorCol],
                &line->text[buf->cursorCol + 1],
                line->length - buf->cursorCol - 1);

        line->length--;
        line->text[line->length] = '\0';
    }
    else if (buf->cursorRow < buf->lineCount - 1)
    {
        // Kasus 2: Merge dengan baris di bawahnya
        Line *nextLine = &buf->lines[buf->cursorRow + 1];

        Line_EnsureCapacity(line, line->length + nextLine->length);
        memcpy(&line->text[line->length], nextLine->text, nextLine->length);
        line->length += nextLine->length;
        line->text[line->length] = '\0';

        Line_Free(nextLine);

        memmove(&buf->lines[buf->cursorRow + 1],
                &buf->lines[buf->cursorRow + 2],
                (buf->lineCount - buf->cursorRow - 2) * sizeof(Line));

        buf->lineCount--;
    }
    // Kalau di akhir baris terakhir → gak ngapa-ngapain
}

// =============================================================
//  Conversion: Buffer ↔ String
// =============================================================

/*
 * Gabungin semua lines jadi 1 string, dipisah "\r\n" (Windows newline).
 * Caller HARUS free() hasilnya.
 *
 * Contoh buffer:
 *   lines[0] = "Hello"
 *   lines[1] = "World"
 *
 * Hasil: "Hello\r\nWorld"
 */
char *Buffer_ToString(TextBuffer *buf)
{
    // Hitung total panjang
    int totalLen = 0;
    for (int i = 0; i < buf->lineCount; i++)
    {
        totalLen += buf->lines[i].length;
        if (i < buf->lineCount - 1)
        {
            totalLen += 2; // "\r\n"
        }
    }

    char *result = (char *)malloc((totalLen + 1) * sizeof(char));
    char *ptr = result;

    for (int i = 0; i < buf->lineCount; i++)
    {
        memcpy(ptr, buf->lines[i].text, buf->lines[i].length);
        ptr += buf->lines[i].length;

        if (i < buf->lineCount - 1)
        {
            *ptr++ = '\r';
            *ptr++ = '\n';
        }
    }
    *ptr = '\0';

    return result;
}

/*
 * Parse string jadi lines di buffer.
 * Handle semua jenis newline: \r\n (Windows), \n (Unix), \r (old Mac)
 *
 * Contoh input: "Hello\r\nWorld\nFoo"
 * Hasil buffer:
 *   lines[0] = "Hello"
 *   lines[1] = "World"
 *   lines[2] = "Foo"
 */
void Buffer_FromString(TextBuffer *buf, const char *str)
{
    Buffer_Clear(buf);

    if (!str || *str == '\0')
        return;

    // Reset — Clear sudah bikin 1 baris kosong
    int row = 0;

    for (const char *p = str; *p != '\0'; p++)
    {
        if (*p == '\r' && *(p + 1) == '\n')
        {
            // Windows newline \r\n → skip both
            p++; // skip \n, loop will advance past \r
            // Tambah baris baru
            Buffer_EnsureLineCapacity(buf, buf->lineCount + 1);
            buf->lineCount++;
            row++;
            Line_Init(&buf->lines[row]);
        }
        else if (*p == '\n' || *p == '\r')
        {
            // Unix \n atau old Mac \r
            Buffer_EnsureLineCapacity(buf, buf->lineCount + 1);
            buf->lineCount++;
            row++;
            Line_Init(&buf->lines[row]);
        }
        else
        {
            // Karakter biasa → append ke baris saat ini
            Line *line = &buf->lines[row];
            Line_EnsureCapacity(line, line->length + 1);
            line->text[line->length] = *p;
            line->length++;
            line->text[line->length] = '\0';
        }
    }

    buf->cursorRow = 0;
    buf->cursorCol = 0;
}