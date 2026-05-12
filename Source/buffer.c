#include "../Header/buffer.h"
#include <stdlib.h>
#include <string.h>

static int Buffer_ClampInt(int value, int minValue, int maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

static TextPos Buffer_ClampPos(const TextBuffer *buf, TextPos pos)
{
    TextPos out = pos;

    if (!buf || buf->lineCount <= 0)
    {
        out.row = 0;
        out.col = 0;
        return out;
    }

    out.row = Buffer_ClampInt(out.row, 0, buf->lineCount - 1);
    out.col = Buffer_ClampInt(out.col, 0, buf->lineLen[out.row]);
    return out;
}

static int Buffer_PosCompare(TextPos a, TextPos b)
{
    if (a.row < b.row)
        return -1;
    if (a.row > b.row)
        return 1;
    if (a.col < b.col)
        return -1;
    if (a.col > b.col)
        return 1;
    return 0;
}

static void Buffer_SetLineEmpty(TextBuffer *buf, int row)
{
    buf->lines[row][0] = '\0';
    buf->lineLen[row] = 0;
}

static void Buffer_ReleaseInitSnapshot(TextBuffer *buf)
{
    if (!buf)
        return;

    free(buf->initSnapshot);
    buf->initSnapshot = NULL;
}

static char *Buffer_DuplicateString(const char *str)
{
    size_t len;
    char *copy;

    if (!str)
        str = "";

    len = strlen(str);
    copy = (char *)malloc(len + 1);
    if (!copy)
        return NULL;

    memcpy(copy, str, len + 1);
    return copy;
}

void Buffer_Init(TextBuffer *buf)
{
    // Minimal 1 baris kosong
    buf->initSnapshot = NULL;
    buf->lineCount = 1;
    buf->cursorRow = 0;
    buf->cursorCol = 0;

    Buffer_SetLineEmpty(buf, 0);
    Buffer_SetInitBuffer(buf);
}

void Buffer_Free(TextBuffer *buf)
{
    // Static buffer: tidak ada free. Reset saja biar konsisten.
    Buffer_ReleaseInitSnapshot(buf);
    Buffer_Clear(buf);
}

void Buffer_Clear(TextBuffer *buf)
{
    buf->lineCount = 1;
    buf->cursorRow = 0;
    buf->cursorCol = 0;
    Buffer_SetLineEmpty(buf, 0);
}

void Buffer_SetInitBuffer(TextBuffer *buf)
{
    char *snapshot;

    if (!buf)
        return;

    snapshot = Buffer_ToString(buf);
    if (!snapshot)
        return;

    Buffer_ReleaseInitSnapshot(buf);
    buf->initSnapshot = snapshot;
}

int Buffer_IsBufferChanged(const TextBuffer *buf)
{
    char *current;
    int changed;

    if (!buf)
        return 0;

    current = Buffer_ToString((TextBuffer *)buf);
    if (!current)
        return 1;

    if (!buf->initSnapshot)
    {
        changed = current[0] != '\0';
    }
    else
    {
        changed = strcmp(current, buf->initSnapshot) != 0;
    }

    free(current);
    return changed;
}

int Buffer_IsBufferSavable(const TextBuffer *buf)
{
    return Buffer_IsBufferChanged(buf);
}

void Buffer_InsertChar(TextBuffer *buf, char c)
{
    int r = buf->cursorRow;
    int ccol = buf->cursorCol;

    if (r < 0 || r >= buf->lineCount)
        return;

    int len = buf->lineLen[r];
    if (len >= BUF_MAX_COLS - 1) // reserve for '\0'
        return;

    if (ccol < 0)
        ccol = 0;
    if (ccol > len)
        ccol = len;

    memmove(&buf->lines[r][ccol + 1],
            &buf->lines[r][ccol],
            (size_t)(len - ccol));

    buf->lines[r][ccol] = c;
    len++;
    buf->lineLen[r] = len;
    buf->lines[r][len] = '\0';

    buf->cursorCol = ccol + 1;
}

void Buffer_InsertNewline(TextBuffer *buf)
{
    int r = buf->cursorRow;
    int ccol = buf->cursorCol;

    if (buf->lineCount >= BUF_MAX_LINES)
        return;

    int len = buf->lineLen[r];
    if (ccol < 0)
        ccol = 0;
    if (ccol > len)
        ccol = len;

    // Shift lines down from bottom to r+1
    for (int i = buf->lineCount; i > r + 1; i--)
    {
        memcpy(buf->lines[i], buf->lines[i - 1], BUF_MAX_COLS);
        buf->lineLen[i] = buf->lineLen[i - 1];
    }

    // New line content = remainder
    int remainLen = len - ccol;
    Buffer_SetLineEmpty(buf, r + 1);

    if (remainLen > 0)
    {
        // Copy from old line remainder
        memcpy(buf->lines[r + 1], &buf->lines[r][ccol], (size_t)remainLen);
        buf->lineLen[r + 1] = remainLen;
        buf->lines[r + 1][remainLen] = '\0';
    }

    // Truncate current line at cursor
    buf->lineLen[r] = ccol;
    buf->lines[r][ccol] = '\0';

    buf->lineCount++;
    buf->cursorRow = r + 1;
    buf->cursorCol = 0;
}

void Buffer_Backspace(TextBuffer *buf)
{
    int r = buf->cursorRow;
    int ccol = buf->cursorCol;

    if (r < 0 || r >= buf->lineCount)
        return;

    if (ccol > 0)
    {
        int len = buf->lineLen[r];

        memmove(&buf->lines[r][ccol - 1],
                &buf->lines[r][ccol],
                (size_t)(len - ccol));

        len--;
        buf->lineLen[r] = len;
        buf->lines[r][len] = '\0';

        buf->cursorCol = ccol - 1;
        return;
    }

    // ccol == 0
    if (r == 0)
        return;

    // Merge with previous line
    int prevLen = buf->lineLen[r - 1];
    int curLen = buf->lineLen[r];

    // If overflow, clamp merge (atau bisa reject). Untuk behaviour "aman", kita merge semampunya.
    int canCopy = curLen;
    if (prevLen + canCopy > BUF_MAX_COLS - 1)
        canCopy = (BUF_MAX_COLS - 1) - prevLen;

    if (canCopy > 0)
    {
        memcpy(&buf->lines[r - 1][prevLen], buf->lines[r], (size_t)canCopy);
        prevLen += canCopy;
        buf->lineLen[r - 1] = prevLen;
        buf->lines[r - 1][prevLen] = '\0';
    }

    // Shift lines up (remove row r)
    for (int i = r; i < buf->lineCount - 1; i++)
    {
        memcpy(buf->lines[i], buf->lines[i + 1], BUF_MAX_COLS);
        buf->lineLen[i] = buf->lineLen[i + 1];
    }

    buf->lineCount--;
    buf->cursorRow = r - 1;
    buf->cursorCol = buf->lineLen[r - 1]; // seperti versi lama: ke akhir prev line (setelah merge)
}

void Buffer_Delete(TextBuffer *buf)
{
    int r = buf->cursorRow;
    int ccol = buf->cursorCol;

    if (r < 0 || r >= buf->lineCount)
        return;

    int len = buf->lineLen[r];

    if (ccol < len)
    {
        memmove(&buf->lines[r][ccol],
                &buf->lines[r][ccol + 1],
                (size_t)(len - ccol - 1));

        len--;
        buf->lineLen[r] = len;
        buf->lines[r][len] = '\0';
        return;
    }

    // ccol == len
    if (r >= buf->lineCount - 1)
        return;

    // Merge next line into current
    int nextLen = buf->lineLen[r + 1];

    int canCopy = nextLen;
    if (len + canCopy > BUF_MAX_COLS - 1)
        canCopy = (BUF_MAX_COLS - 1) - len;

    if (canCopy > 0)
    {
        memcpy(&buf->lines[r][len], buf->lines[r + 1], (size_t)canCopy);
        len += canCopy;
        buf->lineLen[r] = len;
        buf->lines[r][len] = '\0';
    }

    // Remove next line (shift up)
    for (int i = r + 1; i < buf->lineCount - 1; i++)
    {
        memcpy(buf->lines[i], buf->lines[i + 1], BUF_MAX_COLS);
        buf->lineLen[i] = buf->lineLen[i + 1];
    }

    buf->lineCount--;
}

int Buffer_HasSelection(const TextBuffer *buf, const TextSelection *sel)
{
    TextPos start;
    TextPos end;

    if (!buf || !sel || !sel->active)
        return 0;

    Buffer_NormalizeSelection(buf, sel, &start, &end);
    return Buffer_PosCompare(start, end) != 0;
}

void Buffer_NormalizeSelection(const TextBuffer *buf, const TextSelection *sel, TextPos *outStart, TextPos *outEnd)
{
    TextPos start = {0, 0};
    TextPos end = {0, 0};

    if (buf && sel)
    {
        start = Buffer_ClampPos(buf, sel->start);
        end = Buffer_ClampPos(buf, sel->end);

        if (Buffer_PosCompare(start, end) > 0)
        {
            TextPos tmp = start;
            start = end;
            end = tmp;
        }
    }

    if (outStart)
        *outStart = start;
    if (outEnd)
        *outEnd = end;
}

char *Buffer_GetSelectedString(const TextBuffer *buf, const TextSelection *sel)
{
    TextPos start;
    TextPos end;
    int totalLen = 0;

    if (!Buffer_HasSelection(buf, sel))
        return NULL;

    Buffer_NormalizeSelection(buf, sel, &start, &end);

    if (start.row == end.row)
    {
        totalLen = end.col - start.col;
    }
    else
    {
        totalLen += buf->lineLen[start.row] - start.col;
        totalLen += 2; // "\r\n"

        for (int row = start.row + 1; row < end.row; row++)
        {
            totalLen += buf->lineLen[row];
            totalLen += 2; // "\r\n"
        }

        totalLen += end.col;
    }

    char *result = (char *)malloc((size_t)totalLen + 1);
    if (!result)
        return NULL;

    char *ptr = result;

    if (start.row == end.row)
    {
        int len = end.col - start.col;
        if (len > 0)
        {
            memcpy(ptr, &buf->lines[start.row][start.col], (size_t)len);
            ptr += len;
        }
    }
    else
    {
        int firstLen = buf->lineLen[start.row] - start.col;
        if (firstLen > 0)
        {
            memcpy(ptr, &buf->lines[start.row][start.col], (size_t)firstLen);
            ptr += firstLen;
        }
        *ptr++ = '\r';
        *ptr++ = '\n';

        for (int row = start.row + 1; row < end.row; row++)
        {
            int len = buf->lineLen[row];
            if (len > 0)
            {
                memcpy(ptr, buf->lines[row], (size_t)len);
                ptr += len;
            }
            *ptr++ = '\r';
            *ptr++ = '\n';
        }

        if (end.col > 0)
        {
            memcpy(ptr, buf->lines[end.row], (size_t)end.col);
            ptr += end.col;
        }
    }

    *ptr = '\0';
    return result;
}

int Buffer_DeleteSelection(TextBuffer *buf, const TextSelection *sel)
{
    TextPos start;
    TextPos end;

    if (!Buffer_HasSelection(buf, sel))
        return 0;

    Buffer_NormalizeSelection(buf, sel, &start, &end);

    if (start.row == end.row)
    {
        int row = start.row;
        int len = buf->lineLen[row];
        int removeLen = end.col - start.col;

        memmove(&buf->lines[row][start.col],
                &buf->lines[row][end.col],
                (size_t)(len - end.col));

        len -= removeLen;
        buf->lineLen[row] = len;
        buf->lines[row][len] = '\0';
    }
    else
    {
        int startLen = start.col;
        int endTailLen = buf->lineLen[end.row] - end.col;
        int maxLineLen = BUF_MAX_COLS - 1;

        int copyTailLen = endTailLen;
        if (startLen + copyTailLen > maxLineLen)
            copyTailLen = maxLineLen - startLen;

        if (copyTailLen > 0)
        {
            memcpy(&buf->lines[start.row][startLen],
                   &buf->lines[end.row][end.col],
                   (size_t)copyTailLen);
        }

        buf->lineLen[start.row] = startLen + copyTailLen;
        buf->lines[start.row][buf->lineLen[start.row]] = '\0';

        int removeLines = end.row - start.row;
        for (int row = start.row + 1; row + removeLines < buf->lineCount; row++)
        {
            memcpy(buf->lines[row], buf->lines[row + removeLines], BUF_MAX_COLS);
            buf->lineLen[row] = buf->lineLen[row + removeLines];
        }

        buf->lineCount -= removeLines;
        if (buf->lineCount < 1)
            buf->lineCount = 1;
    }

    buf->cursorRow = start.row;
    buf->cursorCol = start.col;

    return 1;
}

char *Buffer_ToString(const TextBuffer *buf)
{
    int totalLen = 0;
    for (int i = 0; i < buf->lineCount; i++)
    {
        totalLen += buf->lineLen[i];
        if (i < buf->lineCount - 1)
            totalLen += 2; // "\r\n"
    }

    char *result = (char *)malloc((size_t)totalLen + 1);
    if (!result)
        return NULL;

    char *ptr = result;
    for (int i = 0; i < buf->lineCount; i++)
    {
        int len = buf->lineLen[i];
        if (len > 0)
        {
            memcpy(ptr, buf->lines[i], (size_t)len);
            ptr += len;
        }

        if (i < buf->lineCount - 1)
        {
            *ptr++ = '\r';
            *ptr++ = '\n';
        }
    }

    *ptr = '\0';
    return result;
}

void Buffer_FromString(TextBuffer *buf, const char *str)
{
    Buffer_Clear(buf);
    if (!str || *str == '\0')
        return;

    int row = 0;

    for (const char *p = str; *p != '\0'; p++)
    {
        if (*p == '\r' && *(p + 1) == '\n')
        {
            // New line
            if (buf->lineCount >= BUF_MAX_LINES)
                break;

            p++; // skip \n
            row++;
            buf->lineCount = row + 1;
            Buffer_SetLineEmpty(buf, row);
        }
        else if (*p == '\n' || *p == '\r')
        {
            if (buf->lineCount >= BUF_MAX_LINES)
                break;

            row++;
            buf->lineCount = row + 1;
            Buffer_SetLineEmpty(buf, row);
        }
        else
        {
            int len = buf->lineLen[row];
            if (len < BUF_MAX_COLS - 1)
            {
                buf->lines[row][len] = *p;
                len++;
                buf->lineLen[row] = len;
                buf->lines[row][len] = '\0';
            }
        }
    }

    buf->cursorRow = 0;
    buf->cursorCol = 0;
}

InsertStringResult Buffer_InsertString(TextBuffer *buf, const char *str, const TextSelection *sel)
{
    InsertStringResult result = {.removed = NULL, .inserted = NULL, .removedLen = 0, .insertedLen = 0};

    if (!buf || !str)
        return result;

    // Step 1: Handle selection - delete dan track removed string
    if (Buffer_HasSelection(buf, sel))
    {
        result.removed = Buffer_GetSelectedString(buf, sel);
        if (result.removed)
            result.removedLen = (int)strlen(result.removed);
        Buffer_DeleteSelection(buf, sel);
    }

    // Step 2: Allocate buffer untuk track inserted string
    // Worst case: semua char jadi 2 bytes (e.g., single \n becomes \r\n)
    int maxPossibleInsert = ((int)strlen(str) * 2) + 1;
    result.inserted = (char *)malloc((size_t)maxPossibleInsert);
    if (!result.inserted)
        return result;

    char *outPtr = result.inserted;
    int outLen = 0;

    // Step 3: Insert string char by char, handling newlines
    for (const char *p = str; *p != '\0'; p++)
    {
        // Check overflow BEFORE inserting
        int currentLineLen = buf->lineLen[buf->cursorRow];

        // If we're at max lines AND current line is full, stop
        if (buf->lineCount >= BUF_MAX_LINES && currentLineLen >= BUF_MAX_COLS - 1)
        {
            break;
        }

        if (*p == '\r' && *(p + 1) == '\n')
        {
            // Windows newline \r\n -> convert to actual newline
            *outPtr++ = '\r';
            *outPtr++ = '\n';
            outLen += 2;

            Buffer_InsertNewline(buf);
            p++; // skip \n, loop will increment p again
        }
        else if (*p == '\n')
        {
            // Unix newline
            *outPtr++ = '\n';
            outLen += 1;

            Buffer_InsertNewline(buf);
        }
        else if (*p == '\r')
        {
            // Mac newline
            *outPtr++ = '\r';
            outLen += 1;

            Buffer_InsertNewline(buf);
        }
        else
        {
            // Regular character
            if (buf->lineLen[buf->cursorRow] >= BUF_MAX_COLS - 1)
            {
                // Current line full, can't add more chars to this line
                // Truncate remaining input (sebagai behaviour default)
                break;
            }

            *outPtr++ = *p;
            outLen += 1;

            Buffer_InsertChar(buf, *p);
        }
    }

    *outPtr = '\0';
    result.insertedLen = outLen;

    return result;
}

void Buffer_FreeInsertStringResult(InsertStringResult *result)
{
    if (!result)
        return;

    if (result->removed)
    {
        free(result->removed);
        result->removed = NULL;
    }

    if (result->inserted)
    {
        free(result->inserted);
        result->inserted = NULL;
    }

    result->removedLen = 0;
    result->insertedLen = 0;
}