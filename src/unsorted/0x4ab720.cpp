// Decompiled by deepseek-v4.1. Names are provisional.
// (earlier work by deepseek-v4.1-flash and GPT-6.1-sol)
//
// STATUS 93.7%, partial. Every byte of the 1015 matches except the scheduling
// of the two FUN_004a5030 calls in the char-insert default case (0x4ab972).
// Original: push ebp / mov [esp+0x30],bl / mov [esp+0x31],0 / call FUN(text) /
//   mov edi,eax / lea eax,[esp+0x2c] / push eax / call FUN(c) / add edi,eax
// Ours:     lea eax,[esp+0x2c] / mov [esp+0x2c],bl / push eax /
//   mov [esp+0x31],0 / call FUN(c) / push ebp / mov edi,eax / call FUN(text) /
//   add edi,eax
// Same 29 bytes, same slots (c sits on the param_3 home at frame+0x2c in both),
// only the evaluation order of the sum differs: the compiler picks the FUN(c)
// subtree first here. Everything before 0x4ab972 matches byte for byte, so the
// allocator state at the block is identical; the choice is made inside the
// width expression.
// Tried and all byte-identical to the current file:
//   FUN(c) + FUN(text) (swapped operands), char c[2] = {(char)key,0},
//   char* cp = c, FUN(&c[0]) on either side, unsigned char c[2],
//   sum inlined in the if condition.
// `int w = FUN(text); int width = w + FUN(c);` reproduces the call order and
// every instruction of 0x4ab972..0x4ab98d exactly, but then the sum lands in
// eax (add eax,edi) and the insert block loses its `mov eax,ecx`
// (0x4ab99e entry-in-eax), and the function comes out 2 bytes short.
// `int width = FUN(text); width += FUN(c);` keeps the size but flips the
// insert block the same way. So the remaining diff is one allocator decision
// (which call is hoisted) that no rewrite of that expression moved.
//
// The char-insert block's `mov eax, ecx` (entry held in ecx, w in edx) only
// appears when the width is one combined expression `FUN(text) + FUN(c)`.
// The char-insert `int i;` local is declared before `char* text` so MSVC
// makes the subscript the addressing-mode index, giving the original
// `[ebp+eax]` instead of `[eax+ebp]` in the copy loops.
// The entry struct must be padded to 0x15b bytes or MSVC scales the index by
// 0x13a (compiler uses sizeof(Entry)).
//
// Text-edit key handler for one GUI entry (stride 0x15b, text at +0xb6).
// __stdcall(control, entryIndex, key): when the holder has no pending
// event source it pulls keys from FUN_004c1ab0. Handles backspace, escape,
// delete, home, end, left, right, clipboard paste and plain character
// insertion, then saves the text back through FUN_004a4d70.
#include <string.h>
#include <windows.h>

#pragma pack(push, 1)
struct Entry_004ab720 {                 // 0x15b bytes
    char unknown_0[0x17];
    short w;                            // +0x17 (max pixel width)
    char unknown_19[2];
    unsigned char flags;                // +0x1b
    char unknown_1c[0xb6 - 0x1c];
    char text[0x80];                    // +0xb6
    char unknown_136[0x138 - 0x136];
    short capacity;                     // +0x138 (max text length)
    char unknown_13a[0x15b - 0x13a];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Holder_004ab720 {
    char unknown_0[4];
    Entry_004ab720* entries;            // +0x4
    char unknown_8[0x18 - 8];
    int field_18;                       // +0x18
};
#pragma pack(pop)

struct Control_004ab720 {
    char unknown_0[0x18];
    Holder_004ab720* holder;            // +0x18
    char unknown_1c[0x74 - 0x1c];
    int cursor;                         // +0x74
};

void __stdcall FUN_004a4d70(Control_004ab720* control, int index);
int __stdcall FUN_004a5030(char* text);
int FUN_004c1ab0();
void* FUN_004b6220();


// FUNCTION: 0x4ab720
int __stdcall FUN_004ab720(Control_004ab720* control, int index, int key)
{
    Holder_004ab720* holder = control->holder;
    Entry_004ab720* entry = &holder->entries[index];
    int i;
    char* text = entry->text;
    int changed = 0;
    int last = 0;

    if (holder->field_18 == 0)
        key = FUN_004c1ab0();
    if (key != 0) {
        changed = 1;
        while (key != 0) {
            last = key;
            int len = strlen(text);
            switch (key) {
            case 0xf1:
                control->cursor = len;
                break;
            case 0xf0:
                control->cursor = 0;
                break;
            case 8:
                if (control->cursor != 0) {
                    control->cursor--;
                    int n = strlen(text);
                    for (i = control->cursor; i < n - 1; i++)
                        text[i] = text[i + 1];
                    text[n - 1] = 0;
                }
                break;
            case 0xf4:
                if (control->cursor != 0)
                    control->cursor--;
                break;
            case 0xf6:
                if (control->cursor < len)
                    control->cursor++;
                break;
            case 0xef:
                if (len != 0 && control->cursor < len) {
                    int n = strlen(text);
                    for (i = control->cursor; i < n - 1; i++)
                        text[i] = text[i + 1];
                    text[n - 1] = 0;
                }
                break;
            case 0xbf:
            case 0xee: {
                HWND hwnd = *(HWND*)((char*)FUN_004b6220() + 0x40);
                if (OpenClipboard(hwnd)) {
                    HANDLE hMem = GetClipboardData(1);
                    if (hMem != 0) {
                        DWORD size = GlobalSize(hMem);
                        if (size != 0) {
                            char* src = (char*)GlobalLock(hMem);
                            memset(text, 0, 0x80);
                            memcpy(text, src, (int)size < entry->capacity - 1 ? (int)size : entry->capacity - 1);
                            int width = FUN_004a5030(text);
                            while (width > entry->w) {
                                if (strlen(text) == 0)
                                    break;
                                text[strlen(text) - 1] = 0;
                                width = FUN_004a5030(text);
                            }
                            GlobalUnlock(hMem);
                        }
                    }
                    CloseClipboard();
                }
                break;
            }
            case 0x1b:
                goto done;
            default: {
                if (len == entry->capacity)
                    break;
                if (key < 0x20 || key > 0x7f)
                    break;
                if (entry->flags & 2) {
                    if (!isalnum(key) && key != '_' && key != ' ' && key != '\'')
                        break;
                }
                char c[2];
                c[0] = (char)key;
                c[1] = 0;
                int width = FUN_004a5030(text) + FUN_004a5030(c);
                if (width > entry->w - 4)
                    break;
                int n = entry->capacity - 1;
                for (i = n; i > control->cursor; i--)
                    text[i] = text[i - 1];
                text[control->cursor] = (char)key;
                control->cursor++;
                break;
            }
            }
            key = FUN_004c1ab0();
        }
    }
done:
    if (changed)
        FUN_004a4d70(control, index);
    return last;
}
