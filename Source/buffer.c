#include "../Header/buffer.h"
#include "../Header/utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static TextLineNode *Buffer_CreateNode(void)
{
    TextLineNode *node = (TextLineNode *)malloc(sizeof(TextLineNode));
    if (!node)
        return NULL;

    node->text[0] = '\0';
    node->len = 0;
    node->isWrapped = 0;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

static TextLineNode *Buffer_CreateWrappedNode(void)
{
    TextLineNode *node = Buffer_CreateNode();
    if (node)
        node->isWrapped = 1;
    return node;
}

static TextLineNode *Buffer_InsertWrappedNodeAfter(TextLineNode *node, TextBuffer *buf)
{
    TextLineNode *newNode;

    if (!node || !buf)
        return NULL;

    newNode = Buffer_CreateWrappedNode();
    if (!newNode)
        return NULL;

    newNode->prev = node;
    newNode->next = node->next;
    if (node->next)
        node->next->prev = newNode;
    else
        buf->tail = newNode;

    node->next = newNode;
    buf->lineCount++;
    return newNode;
}

static void Buffer_MoveCursor(TextBuffer *buf, TextLineNode *node, int row, int col);

// Ensure there is room at the start of node->next to accept one char moved from node.
static void Buffer_MakeRoomAtNext(TextLineNode *node, TextBuffer *buf)
{
    if (!node || !buf)
        return;

    TextLineNode *next = node->next;
    if (!next)
    {
        next = Buffer_CreateWrappedNode();
        if (!next)
            return;
        next->prev = node;
        next->next = NULL;
        node->next = next;
        buf->tail = next;
        buf->lineCount++;
    }

    if (next->len >= BUF_MAX_COLS - 1)
    {
        // next is full, make room recursively first
        Buffer_MakeRoomAtNext(next, buf);
    }

    if (next->len < BUF_MAX_COLS - 1)
    {
        char carry = node->text[node->len - 1];
        memmove(&next->text[1], &next->text[0], (size_t)next->len);
        next->text[0] = carry;
        next->len++;
        next->text[next->len] = '\0';

        node->len--;
        node->text[node->len] = '\0';
    }
}

// After deletions inside a node, pull chars from subsequent wrapped nodes to fill this node
static void Buffer_PullFromNext(TextLineNode *node, TextBuffer *buf)
{
    if (!node || !buf)
        return;

    while (node->len < BUF_MAX_COLS - 1 && node->next && node->next->len > 0 && node->next->isWrapped)
    {
        TextLineNode *next = node->next;
        // move first char from next to end of node
        node->text[node->len] = next->text[0];
        node->len++;
        // shift next left
        if (next->len > 1)
            memmove(&next->text[0], &next->text[1], (size_t)(next->len - 1));
        next->len--;
        next->text[next->len] = '\0';

        // if next became empty and it was a wrapped node, remove it
        if (next->len == 0 && next->isWrapped)
        {
            TextLineNode *after = next->next;
            node->next = after;
            if (after)
                after->prev = node;
            else
                buf->tail = node;
            free(next);
            buf->lineCount--;
        }
        else
        {
            // we've pulled one char, may be able to pull more in next loop iteration
            continue;
        }
    }
}

static TextLineNode *Buffer_EnsureWrappedNext(TextLineNode *node, TextBuffer *buf, int *ioRow)
{
    TextLineNode *target;

    if (!node || !buf)
        return NULL;

    target = node->next;
    if (!target || !target->isWrapped)
    {
        TextLineNode *newNode = Buffer_CreateWrappedNode();
        if (!newNode)
            return NULL;

        newNode->prev = node;
        newNode->next = target;
        if (target)
            target->prev = newNode;
        else
            buf->tail = newNode;

        node->next = newNode;
        buf->lineCount++;
        target = newNode;
    }

    if (ioRow)
        *ioRow = *ioRow + 1;

    return target;
}

static TextLineNode *Buffer_GetRunStartNode(TextLineNode *node)
{
    while (node && node->isWrapped)
        node = node->prev;
    return node;
}

static TextLineNode *Buffer_GetRunEndNode(TextLineNode *node)
{
    while (node && node->next && node->next->isWrapped)
        node = node->next;
    return node;
}

static int Buffer_GetNodeRow(const TextBuffer *buf, const TextLineNode *target)
{
    int row = 0;
    TextLineNode *node;

    if (!buf || !target)
        return 0;

    for (node = buf->head; node; node = node->next, row++)
    {
        if (node == target)
            return row;
    }

    return 0;
}

static void Buffer_FreeWrappedNodesAfter(TextLineNode *node, TextBuffer *buf)
{
    TextLineNode *extra;

    if (!node || !buf)
        return;

    extra = node->next;
    while (extra && extra->isWrapped)
    {
        TextLineNode *next = extra->next;
        free(extra);
        buf->lineCount--;
        extra = next;
    }

    node->next = extra;
    if (extra)
        extra->prev = node;
    else
        buf->tail = node;
}

static void Buffer_ReflowRun(TextBuffer *buf, TextLineNode *anchor, int editOffset, int deleteLen, const char *insertStr)
{
    if (!buf || !anchor)
        return;

    TextLineNode *start = Buffer_GetRunStartNode(anchor);
    TextLineNode *end = Buffer_GetRunEndNode(anchor);
    int row = Buffer_GetNodeRow(buf, start);

    int joinedLen = 0;
    for (TextLineNode *node = start; node; node = node->next)
    {
        joinedLen += node->len;
        if (node == end)
            break;
    }

    int insertLen = insertStr ? (int)strlen(insertStr) : 0;
    char *joined = (char *)malloc((size_t)joinedLen + insertLen + 1);
    if (!joined)
        return;

    int offset = 0;
    for (TextLineNode *node = start; node; node = node->next)
    {
        if (node->len > 0)
        {
            memcpy(&joined[offset], node->text, (size_t)node->len);
            offset += node->len;
        }
        if (node == end)
            break;
    }
    joined[joinedLen] = '\0';

    if (editOffset < 0)
        editOffset = 0;
    if (editOffset > joinedLen)
        editOffset = joinedLen;

    if (deleteLen < 0)
        deleteLen = 0;
    if (editOffset + deleteLen > joinedLen)
        deleteLen = joinedLen - editOffset;

    if (deleteLen > 0)
    {
        memmove(&joined[editOffset], &joined[editOffset + deleteLen], (size_t)(joinedLen - (editOffset + deleteLen)));
        joinedLen -= deleteLen;
    }

    if (insertLen > 0)
    {
        memmove(&joined[editOffset + insertLen], &joined[editOffset], (size_t)(joinedLen - editOffset));
        memcpy(&joined[editOffset], insertStr, (size_t)insertLen);
        joinedLen += insertLen;
    }
    joined[joinedLen] = '\0';

    TextLineNode *node = start;
    TextLineNode *lastUsed = NULL;
    offset = 0;

    if (joinedLen == 0)
    {
        start->text[0] = '\0';
        start->len = 0;
        lastUsed = start;
    }
    else
    {
        while (offset < joinedLen)
        {
            int chunkLen = joinedLen - offset;
            if (chunkLen > BUF_MAX_COLS - 1)
                chunkLen = BUF_MAX_COLS - 1;

            if (!node)
            {
                node = Buffer_InsertWrappedNodeAfter(lastUsed, buf);
                if (!node)
                    break;
            }

            memcpy(node->text, &joined[offset], (size_t)chunkLen);
            node->len = chunkLen;
            node->text[chunkLen] = '\0';
            if (node != start)
                node->isWrapped = 1;

            lastUsed = node;
            offset += chunkLen;

            if (offset >= joinedLen)
                break;

            if (node->next && node->next->isWrapped)
            {
                node = node->next;
                node->isWrapped = 1;
            }
            else
            {
                node = Buffer_InsertWrappedNodeAfter(node, buf);
            }
        }
    }

    if (lastUsed)
        Buffer_FreeWrappedNodesAfter(lastUsed, buf);

    int newAbsoluteOffset = editOffset + insertLen;
    if (newAbsoluteOffset < 0)
        newAbsoluteOffset = 0;
    if (newAbsoluteOffset > joinedLen)
        newAbsoluteOffset = joinedLen;

    int remaining = newAbsoluteOffset;
    TextLineNode *cursorNode = start;
    int cursorRow = row;

    while (cursorNode && remaining > cursorNode->len)
    {
        remaining -= cursorNode->len;
        if (cursorNode->next && cursorNode->next->isWrapped)
        {
            cursorNode = cursorNode->next;
            cursorRow++;
        }
        else
        {
            break;
        }
    }

    if (cursorNode)
    {
        Buffer_MoveCursor(buf, cursorNode, cursorRow, remaining);
    }

    free(joined);
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

    out.row = ClampInt(out.row, 0, buf->lineCount - 1);
    out.col = ClampInt(out.col, 0, Buffer_GetLineLen(buf, out.row));
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

    ccol = ClampInt(buf->cursorCol, 0, node->len);

    if (node->len < BUF_MAX_COLS - 1 && !(node->next && node->next->isWrapped))
    {
        memmove(&node->text[ccol + 1],
                &node->text[ccol],
                (size_t)(node->len - ccol));

        node->text[ccol] = c;
        node->len++;
        node->text[node->len] = '\0';

        Buffer_MoveCursor(buf, node, buf->cursorRow, ccol + 1);
    }
    else
    {
        TextLineNode *start = Buffer_GetRunStartNode(node);
        int offset = 0;
        for (TextLineNode *n = start; n; n = n->next)
        {
            if (n == node)
            {
                offset += ccol;
                break;
            }
            offset += n->len;
        }

        char str[2] = {c, '\0'};
        Buffer_ReflowRun(buf, node, offset, 0, str);
    }
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

    ccol = ClampInt(buf->cursorCol, 0, node->len);

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

    if (newNode->next && newNode->next->isWrapped)
    {
        Buffer_ReflowRun(buf, newNode, 0, 0, NULL);
    }
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

    ccol = ClampInt(buf->cursorCol, 0, node->len);

    if (ccol > 0)
    {
        if (!(node->next && node->next->isWrapped) && !node->isWrapped)
        {
            memmove(&node->text[ccol - 1],
                    &node->text[ccol],
                    (size_t)(node->len - ccol));

            node->len--;
            node->text[node->len] = '\0';
            Buffer_MoveCursor(buf, node, buf->cursorRow, ccol - 1);
            return;
        }

        TextLineNode *start = Buffer_GetRunStartNode(node);
        int offset = 0;
        for (TextLineNode *n = start; n; n = n->next)
        {
            if (n == node)
            {
                offset += ccol;
                break;
            }
            offset += n->len;
        }

        Buffer_ReflowRun(buf, node, offset - 1, 1, NULL);
        return;
    }

    if (buf->cursorRow == 0)
        return;

    {
        TextLineNode *prev = node->prev;
        if (!prev)
            return;

        if (node->isWrapped)
        {
            TextLineNode *start = Buffer_GetRunStartNode(node);
            int offset = 0;
            for (TextLineNode *n = start; n; n = n->next)
            {
                if (n == node)
                    break;
                offset += n->len;
            }
            Buffer_ReflowRun(buf, node, offset - 1, 1, NULL);
        }
        else
        {
            TextLineNode *start1 = Buffer_GetRunStartNode(prev);
            int len1 = 0;
            for (TextLineNode *n = start1; n; n = n->next)
            {
                len1 += n->len;
                if (n == prev)
                    break;
            }

            node->isWrapped = 1;
            Buffer_ReflowRun(buf, start1, len1, 0, NULL);
        }
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

    ccol = ClampInt(buf->cursorCol, 0, node->len);

    TextLineNode *start = Buffer_GetRunStartNode(node);
    int offset = 0;
    for (TextLineNode *n = start; n; n = n->next)
    {
        if (n == node)
        {
            offset += ccol;
            break;
        }
        offset += n->len;
    }

    if (ccol < node->len)
    {
        if (!(node->next && node->next->isWrapped) && !node->isWrapped)
        {
            memmove(&node->text[ccol],
                    &node->text[ccol + 1],
                    (size_t)(node->len - ccol - 1));

            node->len--;
            node->text[node->len] = '\0';
            return;
        }

        Buffer_ReflowRun(buf, node, offset, 1, NULL);
        return;
    }

    if (!node->next)
        return;

    if (node->next->isWrapped)
    {
        Buffer_ReflowRun(buf, node, offset, 1, NULL);
    }
    else
    {
        node->next->isWrapped = 1;
        Buffer_ReflowRun(buf, node, offset, 0, NULL);
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

    TextLineNode *startNode = Buffer_GetLineNodeAtRow(buf, start.row);
    TextLineNode *endNode = Buffer_GetLineNodeAtRow(buf, end.row);
    if (!startNode || !endNode)
        return NULL;

    TextLineNode *node = startNode;
    while (node)
    {
        if (node == startNode && node == endNode)
        {
            totalLen += end.col - start.col;
        }
        else if (node == startNode)
        {
            totalLen += node->len - start.col;
            if (node->next && !node->next->isWrapped)
                totalLen += 2;
        }
        else if (node == endNode)
        {
            totalLen += end.col;
        }
        else
        {
            totalLen += node->len;
            if (node->next && !node->next->isWrapped)
                totalLen += 2;
        }

        if (node == endNode)
            break;
        node = node->next;
    }

    char *result = (char *)malloc((size_t)totalLen + 1);
    if (!result)
        return NULL;

    char *ptr = result;
    node = startNode;
    while (node)
    {
        if (node == startNode && node == endNode)
        {
            int len = end.col - start.col;
            if (len > 0)
            {
                memcpy(ptr, &node->text[start.col], (size_t)len);
                ptr += len;
            }
        }
        else if (node == startNode)
        {
            int len = node->len - start.col;
            if (len > 0)
            {
                memcpy(ptr, &node->text[start.col], (size_t)len);
                ptr += len;
            }
            if (node->next && !node->next->isWrapped)
            {
                *ptr++ = '\r';
                *ptr++ = '\n';
            }
        }
        else if (node == endNode)
        {
            int len = end.col;
            if (len > 0)
            {
                memcpy(ptr, node->text, (size_t)len);
                ptr += len;
            }
        }
        else
        {
            int len = node->len;
            if (len > 0)
            {
                memcpy(ptr, node->text, (size_t)len);
                ptr += len;
            }
            if (node->next && !node->next->isWrapped)
            {
                *ptr++ = '\r';
                *ptr++ = '\n';
            }
        }

        if (node == endNode)
            break;
        node = node->next;
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

    TextLineNode *startNode = Buffer_GetLineNodeAtRow(buf, start.row);
    TextLineNode *endNode = Buffer_GetLineNodeAtRow(buf, end.row);
    if (!startNode || !endNode)
        return 0;

    TextLineNode *startRun = Buffer_GetRunStartNode(startNode);
    TextLineNode *endRun = Buffer_GetRunStartNode(endNode);

    if (startRun == endRun)
    {
        int startOffset = 0;
        for (TextLineNode *n = startRun; n; n = n->next)
        {
            if (n == startNode)
            {
                startOffset += start.col;
                break;
            }
            startOffset += n->len;
        }

        int endOffset = 0;
        for (TextLineNode *n = startRun; n; n = n->next)
        {
            if (n == endNode)
            {
                endOffset += end.col;
                break;
            }
            endOffset += n->len;
        }

        Buffer_ReflowRun(buf, startRun, startOffset, endOffset - startOffset, NULL);
    }
    else
    {
        int startOffset = 0;
        for (TextLineNode *n = startRun; n; n = n->next)
        {
            if (n == startNode)
            {
                startOffset += start.col;
                break;
            }
            startOffset += n->len;
        }

        int endOffset = 0;
        for (TextLineNode *n = endRun; n; n = n->next)
        {
            if (n == endNode)
            {
                endOffset += end.col;
                break;
            }
            endOffset += n->len;
        }

        TextLineNode *endRunLast = Buffer_GetRunEndNode(endNode);
        int endRunLen = 0;
        for (TextLineNode *n = endRun; n; n = n->next)
        {
            endRunLen += n->len;
            if (n == endRunLast)
                break;
        }

        int part2Len = endRunLen - endOffset;
        char *part2 = (char *)malloc((size_t)part2Len + 1);
        if (!part2)
            return 0;

        int dst = 0;
        int copyStarted = 0;
        for (TextLineNode *n = endRun; n; n = n->next)
        {
            if (n == endNode)
            {
                copyStarted = 1;
                int len = n->len - end.col;
                if (len > 0)
                {
                    memcpy(&part2[dst], &n->text[end.col], (size_t)len);
                    dst += len;
                }
            }
            else if (copyStarted)
            {
                if (n->len > 0)
                {
                    memcpy(&part2[dst], n->text, (size_t)n->len);
                    dst += n->len;
                }
            }
            if (n == endRunLast)
                break;
        }
        part2[dst] = '\0';

        TextLineNode *nextFree = startNode->next;
        int freedCount = 0;
        while (nextFree && nextFree != endRunLast->next)
        {
            TextLineNode *temp = nextFree->next;
            free(nextFree);
            freedCount++;
            nextFree = temp;
        }

        startNode->next = nextFree;
        if (nextFree)
            nextFree->prev = startNode;
        else
            buf->tail = startNode;

        buf->lineCount -= freedCount;
        if (buf->lineCount < 1)
            buf->lineCount = 1;

        int startRunLen = 0;
        for (TextLineNode *n = startRun; n; n = n->next)
        {
            startRunLen += n->len;
            if (n == startNode)
                break;
        }

        Buffer_ReflowRun(buf, startRun, startOffset, startRunLen - startOffset, part2);
        free(part2);
    }

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
        if (node->next && !node->next->isWrapped)
            totalLen += 2;
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

        if (node->next && !node->next->isWrapped)
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
            else
            {
                TextLineNode *newNode = Buffer_CreateWrappedNode();
                if (!newNode)
                    break;
                newNode->prev = node;
                newNode->next = NULL;
                node->next = newNode;
                buf->tail = newNode;
                buf->lineCount++;
                node = newNode;
                node->text[node->len] = *p;
                node->len = 1;
                node->text[1] = '\0';
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