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

static TextLineNode *Buffer_CreateNode(void)
{
    TextLineNode *node = (TextLineNode *)malloc(sizeof(TextLineNode));
    if (!node)
        return NULL;

    node->text[0] = '\0';
    node->len = 0;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

static void Buffer_FreeLineList(TextBuffer *buf)
{
    TextLineNode *node;

    if (!buf)
        return;

    node = buf->head;
    while (node)
    {
        TextLineNode *next = node->next;
        free(node);
        node = next;
    }

    buf->head = NULL;
    buf->tail = NULL;
    buf->cursorNode = NULL;
    buf->lineCount = 0;
}

static TextLineNode *Buffer_GetLineNodeAtRow(const TextBuffer *buf, int row)
{
    TextLineNode *node;
    int index;

    if (!buf || row < 0 || row >= buf->lineCount)
        return NULL;

    if (buf->cursorNode)
    {
        int cursorDistance = row - buf->cursorRow;
        if (cursorDistance < 0)
            cursorDistance = -cursorDistance;

        int headDistance = row;
        int tailDistance = buf->lineCount - 1 - row;

        if (cursorDistance <= headDistance && cursorDistance <= tailDistance)
        {
            node = buf->cursorNode;
            index = buf->cursorRow;
        }
        else if (headDistance <= tailDistance)
        {
            node = buf->head;
            index = 0;
        }
        else
        {
            node = buf->tail;
            index = buf->lineCount - 1;
        }
    }
    else
    {
        node = buf->head;
        index = 0;
    }

    while (node && index < row)
    {
        node = node->next;
        index++;
    }

    while (node && index > row)
    {
        node = node->prev;
        index--;
    }

    return node;
}

static void Buffer_ReleaseInitSnapshot(TextBuffer *buf)
{
    if (!buf)
        return;

    free(buf->initSnapshot);
    buf->initSnapshot = NULL;
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
    out.col = Buffer_ClampInt(out.col, 0, Buffer_GetLineLen(buf, out.row));
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

static void Buffer_MoveCursor(TextBuffer *buf, TextLineNode *node, int row, int col)
{
    if (!buf)
        return;

    buf->cursorNode = node;
    buf->cursorRow = row;
    buf->cursorCol = col;
}

int Buffer_GetLineCount(const TextBuffer *buf)
{
    return buf ? buf->lineCount : 0;
}

const char *Buffer_GetLineText(const TextBuffer *buf, int row)
{
    TextLineNode *node = Buffer_GetLineNodeAtRow(buf, row);
    return node ? node->text : "";
}

int Buffer_GetLineLen(const TextBuffer *buf, int row)
{
    TextLineNode *node = Buffer_GetLineNodeAtRow(buf, row);
    return node ? node->len : 0;
}

void Buffer_SetCursorPosition(TextBuffer *buf, int row, int col)
{
    TextPos pos;
    TextLineNode *node;

    if (!buf)
        return;

    if (buf->lineCount <= 0 || !buf->head)
    {
        Buffer_MoveCursor(buf, NULL, 0, 0);
        return;
    }

    pos.row = row;
    pos.col = col;
    pos = Buffer_ClampPos(buf, pos);
    node = Buffer_GetLineNodeAtRow(buf, pos.row);
    Buffer_MoveCursor(buf, node ? node : buf->head, pos.row, pos.col);
}

void Buffer_Init(TextBuffer *buf)
{
    if (!buf)
        return;

    buf->initSnapshot = NULL;
    buf->head = NULL;
    buf->tail = NULL;
    buf->cursorNode = NULL;
    buf->lineCount = 0;
    buf->cursorRow = 0;
    buf->cursorCol = 0;

    Buffer_Clear(buf);
    Buffer_SetInitBuffer(buf);
}

void Buffer_Free(TextBuffer *buf)
{
    if (!buf)
        return;

    Buffer_ReleaseInitSnapshot(buf);
    Buffer_FreeLineList(buf);
}

void Buffer_Clear(TextBuffer *buf)
{
    TextLineNode *node;

    if (!buf)
        return;

    Buffer_FreeLineList(buf);

    node = Buffer_CreateNode();
    if (!node)
    {
        buf->cursorRow = 0;
        buf->cursorCol = 0;
        return;
    }

    buf->head = node;
    buf->tail = node;
    buf->cursorNode = node;
    buf->lineCount = 1;
    buf->cursorRow = 0;
    buf->cursorCol = 0;
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
    TextLineNode *node;
    int ccol;

    if (!buf || buf->lineCount <= 0)
        return;

    node = Buffer_GetLineNodeAtRow(buf, buf->cursorRow);
    if (!node)
        return;

    ccol = Buffer_ClampInt(buf->cursorCol, 0, node->len);
    if (node->len >= BUF_MAX_COLS - 1)
        return;

    memmove(&node->text[ccol + 1],
            &node->text[ccol],
            (size_t)(node->len - ccol));

    node->text[ccol] = c;
    node->len++;
    node->text[node->len] = '\0';

    Buffer_MoveCursor(buf, node, buf->cursorRow, ccol + 1);
}

void Buffer_InsertNewline(TextBuffer *buf)
{
    TextLineNode *node;
    TextLineNode *newNode;
    int ccol;
    int remainLen;
    int nextRow;

    if (!buf || buf->lineCount <= 0)
        return;

    node = Buffer_GetLineNodeAtRow(buf, buf->cursorRow);
    if (!node)
        return;

    ccol = Buffer_ClampInt(buf->cursorCol, 0, node->len);

    newNode = Buffer_CreateNode();
    if (!newNode)
        return;

    remainLen = node->len - ccol;
    if (remainLen > 0)
    {
        memcpy(newNode->text, &node->text[ccol], (size_t)remainLen);
        newNode->len = remainLen;
        newNode->text[remainLen] = '\0';
    }

    node->len = ccol;
    node->text[ccol] = '\0';

    newNode->prev = node;
    newNode->next = node->next;
    if (node->next)
    {
        node->next->prev = newNode;
    }
    else
    {
        buf->tail = newNode;
    }
    node->next = newNode;

    buf->lineCount++;
    nextRow = buf->cursorRow + 1;
    Buffer_MoveCursor(buf, newNode, nextRow, 0);
}

void Buffer_Backspace(TextBuffer *buf)
{
    TextLineNode *node;
    int ccol;

    if (!buf || buf->lineCount <= 0)
        return;

    node = Buffer_GetLineNodeAtRow(buf, buf->cursorRow);
    if (!node)
        return;

    ccol = Buffer_ClampInt(buf->cursorCol, 0, node->len);

    if (ccol > 0)
    {
        memmove(&node->text[ccol - 1],
                &node->text[ccol],
                (size_t)(node->len - ccol));

        node->len--;
        node->text[node->len] = '\0';
        Buffer_MoveCursor(buf, node, buf->cursorRow, ccol - 1);
        return;
    }

    if (buf->cursorRow == 0)
        return;

    {
        TextLineNode *prev = node->prev;
        int prevLen;
        int cursorCol;
        int canCopy;

        if (!prev)
            return;

        prevLen = prev->len;
        cursorCol = prevLen;
        canCopy = node->len;
        if (prevLen + canCopy > BUF_MAX_COLS - 1)
            canCopy = (BUF_MAX_COLS - 1) - prevLen;

        if (canCopy > 0)
        {
            memcpy(&prev->text[prevLen], node->text, (size_t)canCopy);
            prevLen += canCopy;
            prev->len = prevLen;
            prev->text[prevLen] = '\0';
        }

        prev->next = node->next;
        if (node->next)
        {
            node->next->prev = prev;
        }
        else
        {
            buf->tail = prev;
        }

        free(node);
        buf->lineCount--;

        Buffer_MoveCursor(buf, prev, buf->cursorRow - 1, cursorCol);
    }
}

void Buffer_Delete(TextBuffer *buf)
{
    TextLineNode *node;
    int ccol;

    if (!buf || buf->lineCount <= 0)
        return;

    node = Buffer_GetLineNodeAtRow(buf, buf->cursorRow);
    if (!node)
        return;

    ccol = Buffer_ClampInt(buf->cursorCol, 0, node->len);

    if (ccol < node->len)
    {
        memmove(&node->text[ccol],
                &node->text[ccol + 1],
                (size_t)(node->len - ccol - 1));

        node->len--;
        node->text[node->len] = '\0';
        return;
    }

    if (!node->next)
        return;

    {
        TextLineNode *next = node->next;
        int canCopy = next->len;

        if (node->len + canCopy > BUF_MAX_COLS - 1)
            canCopy = (BUF_MAX_COLS - 1) - node->len;

        if (canCopy > 0)
        {
            memcpy(&node->text[node->len], next->text, (size_t)canCopy);
            node->len += canCopy;
            node->text[node->len] = '\0';
        }

        node->next = next->next;
        if (next->next)
        {
            next->next->prev = node;
        }
        else
        {
            buf->tail = node;
        }

        free(next);
        buf->lineCount--;
    }
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
        totalLen += Buffer_GetLineLen(buf, start.row) - start.col;
        totalLen += 2; // "\r\n"

        for (int row = start.row + 1; row < end.row; row++)
        {
            totalLen += Buffer_GetLineLen(buf, row);
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
            memcpy(ptr, &Buffer_GetLineText(buf, start.row)[start.col], (size_t)len);
            ptr += len;
        }
    }
    else
    {
        int firstLen = Buffer_GetLineLen(buf, start.row) - start.col;
        if (firstLen > 0)
        {
            memcpy(ptr, &Buffer_GetLineText(buf, start.row)[start.col], (size_t)firstLen);
            ptr += firstLen;
        }
        *ptr++ = '\r';
        *ptr++ = '\n';

        for (int row = start.row + 1; row < end.row; row++)
        {
            int len = Buffer_GetLineLen(buf, row);
            if (len > 0)
            {
                memcpy(ptr, Buffer_GetLineText(buf, row), (size_t)len);
                ptr += len;
            }
            *ptr++ = '\r';
            *ptr++ = '\n';
        }

        if (end.col > 0)
        {
            memcpy(ptr, Buffer_GetLineText(buf, end.row), (size_t)end.col);
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
        TextLineNode *node = Buffer_GetLineNodeAtRow(buf, start.row);
        int len;
        int removeLen = end.col - start.col;

        if (!node)
            return 0;

        len = node->len;
        memmove(&node->text[start.col],
                &node->text[end.col],
                (size_t)(len - end.col));

        len -= removeLen;
        node->len = len;
        node->text[len] = '\0';
    }
    else
    {
        TextLineNode *startNode = Buffer_GetLineNodeAtRow(buf, start.row);
        TextLineNode *endNode = Buffer_GetLineNodeAtRow(buf, end.row);
        TextLineNode *afterEnd;
        TextLineNode *rowNode;
        int startLen;
        int endTailLen;
        int copyTailLen;
        int removeLines;

        if (!startNode || !endNode)
            return 0;

        startLen = start.col;
        endTailLen = endNode->len - end.col;
        copyTailLen = endTailLen;

        if (startLen + copyTailLen > BUF_MAX_COLS - 1)
            copyTailLen = (BUF_MAX_COLS - 1) - startLen;

        if (copyTailLen > 0)
        {
            memcpy(&startNode->text[startLen],
                   &endNode->text[end.col],
                   (size_t)copyTailLen);
        }

        startNode->len = startLen + copyTailLen;
        startNode->text[startNode->len] = '\0';

        removeLines = end.row - start.row;
        afterEnd = endNode->next;

        rowNode = startNode->next;
        while (rowNode && rowNode != afterEnd)
        {
            TextLineNode *next = rowNode->next;
            free(rowNode);
            rowNode = next;
        }

        startNode->next = afterEnd;
        if (afterEnd)
        {
            afterEnd->prev = startNode;
        }
        else
        {
            buf->tail = startNode;
        }

        buf->lineCount -= removeLines;
        if (buf->lineCount < 1)
            buf->lineCount = 1;
    }

    Buffer_SetCursorPosition(buf, start.row, start.col);
    return 1;
}

char *Buffer_ToString(const TextBuffer *buf)
{
    int totalLen = 0;
    TextLineNode *node;

    if (!buf || buf->lineCount <= 0 || !buf->head)
    {
        char *empty = (char *)malloc(1);
        if (!empty)
            return NULL;

        empty[0] = '\0';
        return empty;
    }

    for (node = buf->head; node; node = node->next)
    {
        totalLen += node->len;
        if (node->next)
            totalLen += 2; // "\r\n"
    }

    char *result = (char *)malloc((size_t)totalLen + 1);
    if (!result)
        return NULL;

    char *ptr = result;
    for (node = buf->head; node; node = node->next)
    {
        if (node->len > 0)
        {
            memcpy(ptr, node->text, (size_t)node->len);
            ptr += node->len;
        }

        if (node->next)
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
    TextLineNode *node;

    if (!buf)
        return;

    Buffer_Clear(buf);
    if (!str || *str == '\0')
        return;

    node = buf->head;

    for (const char *p = str; *p != '\0'; p++)
    {
        if (*p == '\r' && *(p + 1) == '\n')
        {
            TextLineNode *newNode;

            p++; // skip \n
            newNode = Buffer_CreateNode();
            if (!newNode)
                break;

            newNode->prev = node;
            node->next = newNode;
            buf->tail = newNode;
            node = newNode;
            buf->lineCount++;
        }
        else if (*p == '\n' || *p == '\r')
        {
            TextLineNode *newNode = Buffer_CreateNode();
            if (!newNode)
                break;

            newNode->prev = node;
            node->next = newNode;
            buf->tail = newNode;
            node = newNode;
            buf->lineCount++;
        }
        else
        {
            if (node->len < BUF_MAX_COLS - 1)
            {
                node->text[node->len] = *p;
                node->len++;
                node->text[node->len] = '\0';
            }
        }
    }

    Buffer_SetCursorPosition(buf, 0, 0);
}

InsertStringResult Buffer_InsertString(TextBuffer *buf, const char *str, const TextSelection *sel)
{
    InsertStringResult result = {.removed = NULL, .inserted = NULL, .removedLen = 0, .insertedLen = 0};

    if (!buf || !str)
        return result;

    if (Buffer_HasSelection(buf, sel))
    {
        result.removed = Buffer_GetSelectedString(buf, sel);
        if (result.removed)
            result.removedLen = (int)strlen(result.removed);
        Buffer_DeleteSelection(buf, sel);
    }

    int maxPossibleInsert = (int)strlen(str) + 1;
    result.inserted = (char *)malloc((size_t)maxPossibleInsert);
    if (!result.inserted)
        return result;

    char *outPtr = result.inserted;
    int outLen = 0;

    for (const char *p = str; *p != '\0'; p++)
    {
        if (*p == '\r' && *(p + 1) == '\n')
        {
            *outPtr++ = '\n';
            outLen += 1;
            Buffer_InsertNewline(buf);
            p++; // skip \n, loop increments once more
        }
        else if (*p == '\n' || *p == '\r')
        {
            *outPtr++ = '\n';
            outLen += 1;
            Buffer_InsertNewline(buf);
        }
        else
        {
            if (Buffer_GetLineLen(buf, buf->cursorRow) >= BUF_MAX_COLS - 1)
                break;

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