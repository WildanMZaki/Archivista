# Archivista — Final Release Audit

**Date:** 2026-06-03  
**Auditor:** Automated Code Audit  
**Scope:** Full codebase review (all `.c` and `.h` files, build scripts)  
**Verdict:** ⛔ **NOT READY FOR RELEASE** — 9 Critical, 14 High, 12 Medium, 11 Low findings

---

## Severity Legend

| Level | Meaning |
|-------|---------|
| 🔴 **CRITICAL** | Will crash the application, cause data loss, or undefined behavior in normal use |
| 🟠 **HIGH** | Serious bug or security issue reachable under common conditions |
| 🟡 **MEDIUM** | Correctness issue, resource leak, or significant maintainability problem |
| 🟢 **LOW** | Code smell, minor inefficiency, or best-practice violation |

---

## 🔴 CRITICAL Findings

### C01 — Stack Pop is O(n): Undo/Redo will freeze on large documents

**File:** `Source/stack/stack.c` — `DelLast()` (lines 104–138)  
**Category:** Performance / Architecture

The stack is implemented as a singly-linked list where `Push` appends to the **tail** (`InsertLast`) and `Pop` removes from the **tail** (`DelLast`). `DelLast` must traverse the entire list from `bottom` to find the second-to-last node — this is **O(n)** per pop operation.

Every undo/redo keystroke calls `Pop`, and the undo stack can accumulate thousands of entries in a normal editing session. On a 5000-action undo stack, each Ctrl+Z will traverse 5000 nodes. `DeleteAll` (called on redo-stack clear) is **O(n²)** because it pops one-at-a-time.

```c
// DelLast traverses from bottom every single time
while (current->next != NULL) {
    previous = current;
    current = current->next;
}
```

**Impact:** Application freezes for seconds during undo/redo or file operations with long edit histories.

**Fix:** Either:
1. Reverse the stack direction (push/pop at head for O(1)), OR
2. Use a doubly-linked list, OR  
3. Use a dynamic array (realloc-based)

---

### C02 — Unchecked NULL dereference in `GotoEditSubclassProc`

**File:** `Source/goto.c` — lines 6–17  
**Category:** Bug / Crash

```c
LRESULT CALLBACK GotoEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    GotoDlgContext *ctx = (GotoDlgContext *)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
    if (ctx) {
        if (message == WM_KEYDOWN) {
            if (wParam == VK_RETURN) { ... }
        }
    }
    return CallWindowProc(ctx->fnOldEditProc, hWnd, message, wParam, lParam);  // ← ctx may be NULL!
}
```

If `ctx` is NULL (e.g., during window creation before `GWLP_USERDATA` is set), the `CallWindowProc` line dereferences NULL → **crash**.

**Fix:** Move `CallWindowProc` inside the `if (ctx)` block, and return `DefWindowProc` in the else branch.

---

### C03 — `Buffer_MakeRoomAtNext` unbounded recursion → stack overflow

**File:** `Source/buffer.c` — lines 55–90  
**Category:** Bug / Crash

```c
static void Buffer_MakeRoomAtNext(TextLineNode *node, TextBuffer *buf) {
    ...
    if (next->len >= BUF_MAX_COLS - 1) {
        Buffer_MakeRoomAtNext(next, buf);  // ← RECURSIVE
    }
    ...
}
```

When multiple consecutive wrapped nodes are all full (1000 chars each), this recurses one level per full node. A document with a single very long "logical line" (e.g., a minified JSON) could have hundreds of wrapped nodes → hundreds of recursion levels. On the default 1MB Windows stack, this can overflow.

**Note:** `Buffer_ReflowRun` was introduced to handle most of these cases, but `Buffer_MakeRoomAtNext` is still callable from the `Buffer_InsertChar` fast path.

**Fix:** Convert to an iterative approach. Walk the chain forward to find the first node with room, then cascade chars backward.

---

### C04 — `Buffer_IsBufferChanged` is O(N) on every keystroke

**File:** `Source/buffer.c` — `Buffer_IsBufferChanged()` (lines 642–665)  
**Category:** Performance / UX

```c
int Buffer_IsBufferChanged(const TextBuffer *buf) {
    char *current = Buffer_ToString((TextBuffer *)buf);  // ← ALLOCATES entire document as string
    ...
    changed = strcmp(current, buf->initSnapshot) != 0;    // ← compares full strings
    free(current);
    return changed;
}
```

This is called via `App_SyncEditedState` on **every single keystroke** (`Keyboard_OnChar`, line 227). For a 1MB document, every character typed:
1. Allocates ~1MB
2. Copies the entire document
3. Compares ~1MB with `strcmp`
4. Frees ~1MB

**Impact:** Massive performance degradation and allocation churn on documents over a few KB.

**Fix:** Use a dirty flag. Set `isEdited = TRUE` on any edit, `FALSE` on save. Only do the full comparison when the user tries to close/save.

---

### C05 — `strcpy` buffer overflow in `NRLL_Allocation` (Recent files)

**File:** `Source/recent.c` — line 20  
**Category:** Security / Crash

```c
static RecentFileNode *NRLL_Allocation(const char *path) {
    RecentFileNode *node = (RecentFileNode *)malloc(sizeof(RecentFileNode));
    if (node != NULL) {
        strcpy(node->pathFiles, path);  // ← NO bounds check
```

`pathFiles` is `char[MAX_PATH]` (260 bytes). If a file path exceeds `MAX_PATH` (possible with Windows long path support, UNC paths, or malicious `config.ini` contents), this overflows the buffer → heap corruption.

**Fix:** Use `strncpy(node->pathFiles, path, MAX_PATH - 1); node->pathFiles[MAX_PATH - 1] = '\0';`

---

### C06 — `fileops.c:79` — `strcpy` without bounds checking

**File:** `Source/fileops.c` — line 79  
**Category:** Security / Crash

```c
strcpy(s->currentFilePath, path);
```

The `path` argument comes from `Recent_GetRecentPathByIndex()` or from `GetOpenFileName`. While `GetOpenFileName` itself uses `currentFilePath` as its buffer, the recent files path is read from `config.ini` and has no length validation. A crafted/corrupted `config.ini` entry longer than `MAX_PATH` → stack buffer overflow of `currentFilePath[MAX_PATH]`.

**Fix:** Use `strncpy` with `MAX_PATH - 1` limit.

---

### C07 — `Search_ReplaceCurrent` doesn't record undo history

**File:** `Source/search.c` — `Search_ReplaceCurrent()` (lines 192–213)  
**Category:** Bug / Data Loss

```c
void Search_ReplaceCurrent(...) {
    InsertStringResult result = Buffer_InsertString(buf, replaceWith, &s->selection);
    Buffer_FreeInsertStringResult(&result);
    // ← No History_PushAction call!
    s->isEdited = TRUE;
}
```

Replace operations (single and Replace All) bypass the undo system entirely. After using Replace All on a document, the user **cannot undo the changes** — this is data loss.

**Fix:** Record a `HistoryAction` with both the deleted (found) text and inserted (replacement) text, same as the paste handler does.

---

### C08 — History truncation silently loses data

**File:** `Source/history.c` — `History_CreateInsertAction` / `History_CreateDeleteAction`  
**Category:** Bug / Data Loss

```c
if (text && strlen(text) < HISTORY_ACTION_BUFFER_SIZE) {
    strcpy_s(action.add.text, HISTORY_ACTION_BUFFER_SIZE, text);
} else {
    action.add.text[0] = '\0';  // ← ENTIRE text is DISCARDED
}
```

`HISTORY_ACTION_BUFFER_SIZE` is 256 bytes. If a user selects and deletes 257+ characters, the history records an **empty string** for the deleted text. Undo will then re-insert nothing — the text is permanently lost.

The paste handler in `app.c` has a similar issue at lines 339–346 and 353–361 where it truncates.

**Fix:** Use dynamically-allocated strings for the history action text. The fixed-size `char text[256]` design fundamentally cannot support large edits.

---

### C09 — `Search_FindNext` missing braces causes always-return-FALSE

**File:** `Source/search.c` — lines 186–188  
**Category:** Bug

```c
  if (!silent)
    MessageBox(g_searchState.hDlg ? g_searchState.hDlg : NULL, ...);
    return FALSE;  // ← THIS IS NOT INSIDE THE IF! It always executes.
```

Due to missing braces, `return FALSE` is **unconditional** — it executes regardless of `silent`. This means the function's return value is wrong only when it should return from a successful match (which already returned TRUE above), so the effect is that the "not found" message is shown to the user even in silent mode when wrapping fails. However, the actual return value is correct since successful matches return early.

Wait — re-examining: the `return FALSE` is reached only when no match was found across the entire document. The `if (!silent)` only gates the `MessageBox`. The `return FALSE` always runs. This is actually correct behavior (return FALSE when nothing found), but the indentation misleadingly suggests `return FALSE` is inside the `if`. Downgraded to note: the dangling `return FALSE` outside the if is correct, but the indentation is misleading and should use braces.

**Revised Severity: 🟡 MEDIUM** — cosmetic but confusing. The actual logical bug is that in `Search_ReplaceAll`, when `silent=TRUE` the `MessageBox` is correctly suppressed, so this works as intended. But the style is a ticking time bomb.

**Fix:** Add braces around the if-body:
```c
if (!silent) {
    MessageBox(...);
}
return FALSE;
```

---

## 🟠 HIGH Findings

### H01 — `EditHistory` is never initialized

**File:** `Source/app.c` — `App_OnCreate()` (line 78)  
**Category:** Bug

```c
AppState *s = (AppState *)calloc(1, sizeof(AppState));
// ...
// History_Init(&s->history) is NEVER called!
```

`calloc` zeroes memory, which sets `undoStack.bottom = NULL` and `undoStack.top = NULL` — this happens to work because `CreateStack` does the same thing. But this is fragile: if `Stack` ever gains initialization that isn't just zeroing, it will break silently.

**Fix:** Add `History_Init(&s->history);` in `App_OnCreate`.

---

### H02 — `FileOps_Open` with non-NULL path skips cursor reset and UI refresh

**File:** `Source/fileops.c` — lines 84–103  
**Category:** Bug

```c
BOOL FileOps_Open(HWND hWnd, AppState *s, char *path) {
    if (!ConfirmSave(hWnd, s)) return FALSE;
    if (path != NULL) return FileOps_OpenFile(hWnd, s, path);  // ← returns here!
    // ...
    Cursor_SetPosition(&s->textBuffer, 0, 0);  // ← never reached for recent files
    App_RefreshEditorAfterAction(hWnd, s);       // ← never reached for recent files
    App_SyncEditedState(s);                      // ← never reached for recent files
    return TRUE;
}
```

When opening from "recent files", the function returns immediately after `FileOps_OpenFile`. The cursor position is not reset (it uses whatever `Buffer_FromString` leaves), and the scrollbars/display are not updated.

**Fix:** Move the `Cursor_SetPosition`, `App_RefreshEditorAfterAction`, and `App_SyncEditedState` calls into `FileOps_OpenFile`, or call them before the early return.

---

### H03 — History is never cleared on New/Open

**File:** `Source/fileops.c` — `FileOps_New()`, `FileOps_OpenFile()`  
**Category:** Bug

When creating a new file or opening a different file, the undo/redo stacks from the **previous** document remain. Undoing after opening a new file will try to apply edits from the old document to the new one → corrupted text or crash.

**Fix:** Add `History_Clear(&s->history);` in `FileOps_New` and `FileOps_OpenFile`.

---

### H04 — `GetFileSize` with `NULL` high-DWORD rejects files > 4GB and misses errors

**File:** `Source/fileops.c` — line 55  
**Category:** Bug / Security

```c
DWORD fileSize = GetFileSize(hFile, NULL);
if (fileSize == INVALID_FILE_SIZE) { ... }
```

`INVALID_FILE_SIZE` is `0xFFFFFFFF`, which is also a valid file size for a ~4GB file. Without checking `GetLastError()`, this conflates legitimate 4GB files with errors. Also, `malloc(fileSize + 1)` with `fileSize = 0xFFFFFFFF` → `malloc(0)` due to integer overflow.

**Fix:** Use `GetFileSizeEx` which returns a 64-bit size, or check `GetLastError()` after `GetFileSize`.

---

### H05 — `FileOps_WriteToPath` NULL dereference if `Buffer_ToString` fails

**File:** `Source/fileops.c` — lines 115–116  
**Category:** Bug / Crash

```c
char *strBuf = Buffer_ToString(&s->textBuffer);
if (!WriteFile(hFile, strBuf, strlen(strBuf), &bytesWritten, NULL)) {
```

If `Buffer_ToString` returns NULL (OOM), `strlen(NULL)` → crash.

**Fix:** Check `strBuf` for NULL before using it.

---

### H06 — `Clipboard_Copy` uses `strlen` without including `<string.h>`

**File:** `Source/clipboard.c` — line 8  
**Category:** Bug / Portability

```c
int len = strlen(stringIn);  // implicit declaration in C99 mode
```

`clipboard.c` only includes `../Header/clipboard.h` which only includes `<windows.h>`. The `strlen` call has no prototype — this is **undefined behavior** in C99 (`-std=c99` is set in CMake). On x64, the implicit `int` return type truncates the 64-bit pointer return.

Additionally, `memcpy` (line 24) is also used without a proper include.

**Fix:** Add `#include <string.h>` to `clipboard.c`.

---

### H07 — `config.c:GetConfigIniPath` uses `strcpy` / `strrchr` without `<string.h>`

**File:** `Source/config.c` — lines 6, 8, 10  
**Category:** Bug / Portability

Same as H06 — `strcpy` and `strrchr` used without including `<string.h>`. Implicit function declarations are UB in C99.

**Fix:** Add `#include <string.h>` to `config.c`.

---

### H08 — `recent.c:Recent_AddRecent` buffer overflow in `keyName`

**File:** `Source/recent.c` — line 149  
**Category:** Security

```c
char keyName[6];               // Can hold "File" + 1 digit + '\0' = 6
sprintf(keyName, "File%d", i); // i ranges 1..5, "File5" = 6 chars including \0
```

This is exactly at the boundary. If `MAX_RECENT_FILES` is ever increased beyond 9 (e.g., to 10), `"File10"` needs 7 bytes → overflow. Additionally, line 120 uses `char keyName[16]` for the same purpose — inconsistent sizing.

**Fix:** Unify to `char keyName[16]` everywhere, or use `snprintf`.

---

### H09 — `Scroll_GetLongestLineLen` is O(n) per scroll update

**File:** `Source/scroll.c` — lines 5–22  
**Category:** Performance

This iterates over **every line** in the document to find the longest. It's called from `Scroll_UpdateScrollbars` → called from `App_RefreshEditorAfterAction` → called on **every keystroke**.

For a 10,000-line document, every character typed triggers a full scan of all lines.

**Fix:** Cache the longest line length in `TextBuffer` and update it incrementally on edits.

---

### H10 — `Keyboard_CopyHistoryText` is duplicated

**File:** `Source/keyboard.c` (lines 28–59) and `Source/app.c` (lines 17–48)  
**Category:** Dead Code / Maintainability

The exact same function exists as `Keyboard_CopyHistoryText` and `App_CopyHistoryText`. This violates DRY and means bug fixes must be applied in two places.

**Fix:** Extract to a shared utility function.

---

### H11 — `ID_EDIT_DELETE` via menu doesn't record undo history

**File:** `Source/app.c` — lines 379–390  
**Category:** Bug / Data Loss

```c
case ID_EDIT_DELETE:
    if (Buffer_DeleteSelection(&s->textBuffer, &s->selection)) {
        s->selection.active = 0;
    } else {
        Buffer_Delete(&s->textBuffer);
    }
    // ← No History_PushAction call!
```

Pressing Delete via the Edit menu bypasses undo entirely. Compare with the `VK_DELETE` handler in `keyboard.c` (lines 328–378) which does record history.

**Fix:** Replicate the undo logic from `Keyboard_OnKeyDown`'s VK_DELETE handler, or consolidate into a shared function.

---

### H12 — `VK_DELETE` handler shadows outer `row`/`col` variables

**File:** `Source/keyboard.c` — lines 328–331  
**Category:** Bug

```c
case VK_DELETE:
{
    int row = s->textBuffer.cursorRow;   // ← shadows the outer 'row' at line 241
    int col = s->textBuffer.cursorCol;   // ← shadows the outer 'col' at line 242
    ...
    break;
}
// After the switch:
Cursor_SetPosition(&s->textBuffer, row, col);  // ← uses the OUTER row/col!
```

After VK_DELETE, `Cursor_SetPosition` is called with the **original** row/col (from before the delete), not the post-delete position. This could move the cursor to a stale position after a delete operation.

**Fix:** Either `return 0` from within the VK_DELETE case, or don't shadow the variables.

---

### H13 — `FileOps_WriteToPath` truncates files > 4GB

**File:** `Source/fileops.c` — line 116  
**Category:** Bug

```c
if (!WriteFile(hFile, strBuf, strlen(strBuf), &bytesWritten, NULL)) {
```

`strlen` returns `size_t` but `WriteFile` takes `DWORD` (32-bit). For buffers larger than 4GB, this silently truncates. While unlikely for a text editor, the `Buffer_ToString` result length is not validated.

**Fix:** For robustness, cast explicitly and/or add a size check with a warning.

---

### H14 — `App_OnCommand` returns 0 for unhandled commands

**File:** `Source/app.c` — line 473  
**Category:** Bug

```c
LRESULT App_OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    ...
    default:
        if (LOWORD(wParam) >= ID_FILE_RECENT_START && ...) { ... }
    }
    return 0;  // ← Swallows ALL unhandled WM_COMMAND
}
```

Unhandled `WM_COMMAND` messages should be passed to `DefWindowProc`, not swallowed. This could prevent Windows controls from working correctly.

**Fix:** `return DefWindowProc(hWnd, WM_COMMAND, wParam, lParam);`

---

## 🟡 MEDIUM Findings

### M01 — `Buffer_ReflowRun` allocates and joins all text into one string

**File:** `Source/buffer.c` — lines 216–411  
**Category:** Performance

On every character insertion in a wrapped run, the entire logical line is joined into a heap-allocated string, edited, and redistributed. For a 100KB single line (common with minified files), every keystroke allocates and copies 100KB.

**Fix:** For single-char inserts, only reflow the affected node and its neighbors — avoid the full join/redistribute.

---

### M02 — `isStackEmpty` takes `Stack` by value

**File:** `Source/stack/stack.h` — line 27  
**Category:** Performance / C Best Practice

```c
bool isStackEmpty(Stack S);  // copies the struct each call
```

This copies the entire `Stack` struct on every call. While small (2 pointers), this is unconventional and wasteful in C. All other functions take `Stack*`.

**Fix:** Change to `bool isStackEmpty(const Stack *S);`

---

### M03 — No `WM_ERASEBKGND` handler — potential flash on resize

**File:** `Source/winproc.c`  
**Category:** UX

The app does double-buffering in `WM_PAINT`, but doesn't handle `WM_ERASEBKGND`. Windows will erase the background before paint, causing a white flash on resize.

**Fix:** Add `case WM_ERASEBKGND: return 1;` to suppress the default erase.

---

### M04 — `Selection_SetSelection` has no NULL check

**File:** `Source/selection.c` — line 4  
**Category:** Bug / Crash (defensive)

```c
void Selection_SetSelection(AppState *s, int active, ...) {
    s->selection.active = active;  // ← crashes if s is NULL
```

While callers currently check `s` before calling, this is fragile.

**Fix:** Add `if (!s) return;` guard.

---

### M05 — Rendering calls `Buffer_GetLineText` 3 times per line for selections

**File:** `Source/render.c` — lines 115, 122, 131  
**Category:** Performance

Each selected line calls `Buffer_GetLineText` up to 3 times (before selection, selection, after selection). Each call traverses the linked list to find the line node.

**Fix:** Cache the line pointer at the top of the loop iteration.

---

### M06 — `Search_ReplaceAll` may cause integer overflow in message

**File:** `Source/search.c` — lines 232–233  
**Category:** Bug (minor)

```c
char msg[64];
sprintf(msg, "Berhasil mengganti %d kata.", counter);
```

If `counter` is very large (millions of replacements), this could overflow the 64-byte buffer. Use `snprintf`.

**Fix:** `snprintf(msg, sizeof(msg), "Replaced %d occurrences.", counter);`

---

### M07 — `config.ini` path is next to `.exe` — not user-writable on Program Files

**File:** `Source/config.c` — `GetConfigIniPath()`  
**Category:** Deployment / Security

The config file is written next to the executable. If the app is installed in `C:\Program Files\`, writes will fail silently due to UAC/filesystem virtualization, or the file will be virtualized per-user in `VirtualStore`.

**Fix:** Use `%APPDATA%\Archivista\config.ini` via `SHGetFolderPath(CSIDL_APPDATA)`.

---

### M08 — `Recent_FreeAllNode` uses `NRLL_DeleteNode` which modifies the list during traversal

**File:** `Source/recent.c` — lines 106–114  
**Category:** Bug (latent)

```c
void Recent_FreeAllNode() {
    RecentFileNode *curr = recents.First;
    while (curr != NULL) {
        RecentFileNode *next = curr->next;
        NRLL_DeleteNode(&recents, curr);  // ← modifies curr->prev->next and curr->next->prev
        curr = next;
    }
    recents.First = NULL;
}
```

`NRLL_DeleteNode` modifies `curr->next->prev`, which points back to the freed `curr` node. The `next` pointer was saved beforehand so iteration itself is safe, but the `prev` pointer of `next` now points to freed memory. This works only because the `prev` pointer is never read during deletion. Fragile.

**Fix:** Simplify to a plain traversal without `NRLL_DeleteNode`:
```c
while (curr) { RecentFileNode *next = curr->next; free(curr); curr = next; }
recents.First = NULL; recents.recentFileCount = 0;
```

---

### M09 — `Buffer_FromString` does not respect `wrapCols` setting

**File:** `Source/buffer.c` — lines 1248–1317  
**Category:** Bug

When loading a file with word wrap enabled, `Buffer_FromString` always wraps at `BUF_MAX_COLS - 1` (1000) regardless of the current `wrapCols` setting. The text will appear incorrectly until a manual `Buffer_ReflowAll` is triggered.

`FileOps_OpenFile` does not call `Buffer_ReflowAll` after `Buffer_FromString`.

**Fix:** Call `Buffer_ReflowAll` in `FileOps_OpenFile` after `Buffer_FromString` when word wrap is enabled.

---

### M10 — `ConfirmSave` unreachable `break` after `return`

**File:** `Source/fileops.c` — line 28  
**Category:** Dead Code

```c
case IDYES:
    return FileOps_Save(hWnd, s);
    break;  // ← Dead code
```

**Fix:** Remove the unreachable `break`.

---

### M11 — No DPI awareness

**File:** `Source/main.c`, `Source/window.c`  
**Category:** UX

The application doesn't declare DPI awareness. On high-DPI displays (common on modern laptops), the window will appear blurry due to Windows DPI virtualization scaling.

**Fix:** Add a DPI awareness manifest or call `SetProcessDPIAware()` / `SetProcessDpiAwareness()` at startup.

---

### M12 — Title bar never shows the current filename

**File:** entire codebase  
**Category:** UX

Standard text editors show "filename - AppName" or "filename* - AppName" (when modified) in the title bar. Archivista always shows just "Archivista". Users cannot tell which file is open or whether changes are unsaved.

**Fix:** Call `SetWindowText(hWnd, title)` with the current filename after open/save/edit operations.

---

## 🟢 LOW Findings

### L01 — `PrintStack` debug function included in release build

**File:** `Source/stack/stack.c` — lines 141–196  
**Category:** Dead Code

`PrintStack` uses `printf` and a fixed `nodes[100]` array. It's debug-only code that:
1. Adds unnecessary code to the binary
2. Has a hardcoded limit of 100 stack elements
3. Uses `printf` which is unusual in a Win32 GUI app

**Fix:** Wrap in `#ifdef DEBUG` or remove entirely.

---

### L02 — `CMakeLists.txt` requires CMake 4.1

**File:** `CMakeLists.txt` — line 1  
**Category:** Build

```cmake
cmake_minimum_required(VERSION 4.1)
```

CMake 4.1 is extremely new. Most CI systems and developer machines will have CMake 3.x. The project doesn't use any CMake 4.x features.

**Fix:** Lower to `cmake_minimum_required(VERSION 3.15)` or similar.

---

### L03 — Mixed language in comments and user-facing strings

**Files:** Throughout the codebase  
**Category:** Maintainability / i18n

Comments and message strings alternate between Indonesian and English. User-facing messages like `"Berhasil mengganti %d kata."` and `"Teks tidak ditemukan."` are in Indonesian, while others like `"Failed to open file"` are in English.

**Fix:** Standardize on one language (English for international release) and extract all user-facing strings to a central location for future localization.

---

### L04 — Header include paths use relative `../Header/` prefix

**Files:** All `.c` files  
**Category:** Maintainability

Every source file uses `#include "../Header/app.h"` style paths. This makes the code fragile if the directory structure changes.

**Fix:** Add `Header/` to the CMake include path:
```cmake
target_include_directories(Archivista PRIVATE Header)
```
Then use `#include "app.h"` everywhere.

---

### L05 — `Cursor_SetPosition` is a trivial wrapper

**File:** `Source/cursor.c` — lines 64–70  
**Category:** Dead Code / Unnecessary Indirection

```c
void Cursor_SetPosition(TextBuffer *buf, int row, int col) {
    if (!buf) return;
    Buffer_SetCursorPosition(buf, row, col);
}
```

This adds nothing over calling `Buffer_SetCursorPosition` directly (which already does NULL checking and clamping).

**Fix:** Consider removing and calling `Buffer_SetCursorPosition` directly, or document why the indirection exists.

---

### L06 — `NRLL_InsertLast` is dead code

**File:** `Source/recent.c` — lines 69–84  
**Category:** Dead Code

`NRLL_InsertLast` is only called from `Recent_LoadRecent`. The same functionality could use `NRLL_InsertFirst` with a reverse iteration, making `InsertLast` unnecessary.

Actually, `InsertLast` is used to maintain load order. This is not strictly dead code — **revised to: acceptable**.

---

### L07 — `utils.h` uses non-namespaced include guard

**File:** `Header/utils.h` — line 1  
**Category:** Code Smell

```c
#ifndef UTILS_H  // ← generic, likely to collide with other libraries
```

All other headers use `ARCHIVISTA_*` prefix.

**Fix:** Rename to `ARCHIVISTA_UTILS_H`.

---

### L08 — `static inline` function in header may cause unused-function warnings

**File:** `Header/utils.h` — `ClampInt`  
**Category:** Code Smell

Some compilers warn about `static inline` functions defined in headers when they're included but not called in a translation unit.

**Fix:** This is generally acceptable in C99, but could be moved to a `.c` file if warnings appear.

---

### L09 — `AppState.isComposingAction` and `currentAction` are unused

**File:** `Header/app.h` — lines 32–33  
**Category:** Dead Code

```c
BOOL isComposingAction;
HistoryAction currentAction;
```

These fields are declared but never read or written anywhere in the codebase. They appear to be planned for multi-character action composition but were never implemented.

**Fix:** Remove or implement.

---

### L10 — Missing `#include <ctype.h>` in `selection.c`

**File:** `Source/selection.c` — line 26  
**Category:** Portability

`isalnum()` is used without including `<ctype.h>`. This may work because `<windows.h>` transitively includes it on MSVC/MinGW, but it's not guaranteed.

**Fix:** Add `#include <ctype.h>`.

---

### L11 — `#pragma comment(lib, "Shlwapi.lib")` is MSVC-only

**File:** `Source/search.c` — line 7  
**Category:** Portability

This pragma is ignored by GCC/MinGW (the project's target compiler). The Shlwapi library must be linked via CMake instead.

**Fix:** Add `target_link_libraries(Archivista Shlwapi)` to `CMakeLists.txt` and remove the pragma.

---

## Architecture Concerns

### A01 — No test suite

There are zero automated tests. For a text editor shipping to users, the buffer manipulation logic (`Buffer_InsertChar`, `Buffer_Backspace`, `Buffer_Delete`, `Buffer_InsertNewline`, `Buffer_DeleteSelection`, `Buffer_InsertString`, `Buffer_ReflowRun`) is complex enough to warrant unit tests. The linked list manipulation is especially error-prone.

### A02 — Global mutable state (`recents`, `g_searchState`, `s_lastDblClkTime`)

Multiple modules use file-scope static or global variables. This makes the code difficult to test and prevents running multiple editor instances in the same process.

### A03 — No separation between model and view

The `TextBuffer` (model) and rendering/scrolling (view) are tightly coupled through `AppState`. Functions like `Search_FindNext` mix search logic with UI updates (`Selection_SetSelection`, `App_RefreshEditorAfterAction`). This makes the buffer logic untestable in isolation.

### A04 — `TextLineNode` uses fixed 1001-byte arrays

Each line node is `sizeof(TextLineNode)` ≈ 1024 bytes regardless of actual content. A 10,000-line file with average 40-char lines wastes ~9.6MB of memory (1001 bytes allocated × 10000, vs ~400KB actual content). Consider dynamic allocation for line text.

---

## Pre-Release Checklist

| Item | Status |
|------|--------|
| All critical bugs fixed | ❌ |
| All high-severity bugs fixed | ❌ |
| Undo/redo works for all edit operations | ❌ (Replace, menu Delete) |
| No silent data truncation in undo | ❌ (256-byte limit) |
| No buffer overflows from external input | ❌ (config.ini, file paths) |
| Performance acceptable for 10K+ line files | ❌ (O(n) per keystroke) |
| DPI-aware rendering | ❌ |
| Title bar shows current file | ❌ |
| All compiler warnings resolved | ❓ (missing includes) |
| Automated tests exist | ❌ |
| Consistent UI language | ❌ |

---

## Recommended Fix Priority

**Before release (must-fix):**
1. **C02** — NULL crash in goto dialog (5 min fix)
2. **C04** — O(n) `Buffer_IsBufferChanged` per keystroke (30 min fix)
3. **C07** — Replace doesn't record undo (1 hour fix)
4. **C09** — Add braces in `Search_FindNext` (2 min fix)
5. **H02** — Recent file open skips UI refresh (5 min fix)
6. **H03** — Clear history on New/Open (5 min fix)
7. **H05** — NULL check in `FileOps_WriteToPath` (2 min fix)
8. **H06/H07** — Missing `<string.h>` includes (2 min fix)
9. **H11** — Menu Delete doesn't record undo (15 min fix)

**Next sprint (should-fix):**
1. **C01** — O(n) stack pop (1 hour refactor)
2. **C05/C06** — `strcpy` overflow prevention (15 min fix)
3. **C08** — Dynamic history text allocation (2 hour refactor)
4. **H09** — Cache longest line length (30 min fix)
5. **M03** — `WM_ERASEBKGND` handler (2 min fix)
6. **M12** — Title bar shows filename (15 min fix)

**Future improvement:**
1. **A01** — Unit test suite for buffer operations
2. **C03** — Iterative `Buffer_MakeRoomAtNext`
3. **M07** — AppData config path
4. **M11** — DPI awareness
5. **A04** — Dynamic line text allocation
