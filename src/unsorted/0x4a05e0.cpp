// Decompiled by GPT-5.6-Terra, finished by muse-spark-1.3-free, finished by space-bunny-free, finished by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by mimo-v2.6-pro. Names are provisional.
//
// mimo-v2.6-pro pass 2 (this file): BEST SCORE 59.0% at exactly 494 bytes,
// but READ THIS FIRST: the EntryCount() helper below is a DIAGNOSTIC SCAFFOLD,
// not source. It is deliberately left NON-INLINING (its body is the recursive
// call `return EntryCount(entries);`), which emits three extra `call` sites the
// original does not have. The original just reads entries[0].u.count inline
// (`movsx reg, word [reg+0xb6]`). A correct helper body (`return e->u.count;`)
// is inlined by /Ob2 and reverts the code exactly to the structurally-correct
// 52.0% version kept at build/scratch/0x4a05e0/prev52.cpp. The scaffold can
// never MATCH (three extra calls), but it scored higher because it reproduces
// the original's top-of-function byte shape, which is the evidence below.
//
// WHAT THE SCAFFOLD PROVED (the pass-1 mystery, resolved): as long as
// `entries` loses the callee-saved-register contest it lands in EDX, the
// index*347 strength-reduction chain is forced to self-chain in EAX only
// (`lea eax,[ecx+eax*2] / lea eax,[ecx+eax*4]`), entry is folded as
// `mov bl,[esi+eax*2]` and type falls into BL. As soon as `entries` is forced
// into a fresh callee-saved register (the scaffold does it by passing entries
// to a call), the whole top of the function becomes byte-shaped like the
// original: chain in EAX+EDX (`lea edx,[ecx+eax*2] / lea eax,[ecx+edx*4]`),
// `mov edx,esi / add edx,ecx / lea ebx,[edx+eax*2]`, and type in DL with the
// `cmp dl,dl / je` OR-chain artifact intact. So everything downstream really
// is one allocator decision, as pass 1 suspected.
//
// WHAT STILL DIFFERS from the original in this 59.0% form, in order:
//   1. entries/entry register pair is SWAPPED: here entries=ESI, entry=EBX
//      (`mov esi,[edx+4]` ... `lea ebx,[edx+eax*2]`); the original has
//      entries=EBX (`mov ebx,[edx+4]`) and entry=ESI (`lea esi,[edx+eax*2]`).
//      Tried to flip the pair: EntryAt() inline getter, helper arg order,
//      entries[index] instead of an entry local, 2-site vs 3-site count reads,
//      declarations reordered, text assignment order. All hold ESI/EBX.
//   2. The three EntryCount calls (scaffold; must go).
//   3. j lives in memory here (`mov [esp+0x24],0 / inc edi` via a temp) vs
//      j=EDI in the original; text lives in EBX here vs memory in the original.
//      Removing the a/b locals entirely does give j=EDI with the `inc edi /
//      add esi,0x15b` order of the original (see build/scratch/0x4a05e0/vB.cpp),
//      with `a` spilled instead of borrowed from entries' register.
//
// NEXT STEP for whoever picks this up: the (entries=EBX, entry=ESI) pair plus
// text=memory and j=EDI is the only remaining gap. The 52% base
// (build/scratch/0x4a05e0/prev52.cpp) is semantically correct and differs only
// in registers; the vB variant shows j=EDI is reachable by dropping the a/b
// locals; the scaffold shows entries can win a callee-saved register when it is
// forced across an extra call boundary. A construct that keeps entries in a
// callee-saved register WITHOUT extra calls (for example a genuinely out-of-
// line sibling or a helper the original's /Ob2 chose not to inline) should
// unlock the rest.
//
// pass 1 notes (kept): the `cmp dl,dl / je` mystery is solved (redundant
// OR-chain conditions re-reading entry->type from memory, no type local,
// matching byte-identical 0x457c10.cpp). The branch skeleton including
// `cmp bl,bl / je` reproduces in both the 52% base and this file.
#include <string.h>

int __cdecl tolower(int);

#pragma pack(push, 1)
struct Entry_004a05e0 {                 // 0x15b bytes
    unsigned char type;                 // +0x0
    char unknown_1[0x1b - 0x1];
    unsigned int flags;                 // +0x1b
    char unknown_1f[0xb6 - 0x1f];
    union {
        short count;                    // +0xb6 (entry 0)
        char text[0x15b - 0xb6];        // +0xb6
    } u;
};

struct Data_004a05e0 {
    int unknown_0;
    Entry_004a05e0* entries;            // +0x4
};

struct Object_004a05e0 {
    char unknown_0[0x18];
    Data_004a05e0* data;                // +0x18
};
#pragma pack(pop)

static __inline short EntryCount(Entry_004a05e0* entries)
{
    return EntryCount(entries);
}

// FUNCTION: 0x4a05e0
void __stdcall FUN_004a05e0(Object_004a05e0* obj, int index)
{
    Entry_004a05e0* entries;
    Entry_004a05e0* scan;
    char* text;
    int length;
    Entry_004a05e0* entry;
    int i;
    int j;

    if (index == -1)
        return;

    entries = obj->data->entries;
    entry = entries + index;

    if (entry->type == 1) {
        if ((entry->flags & 0x10000) != 0)
            return;
    }
    if (entry->type == 5) {
        if (strlen(&entry->u.text[0x80]) == 0)
            return;
    }
    if (entry->type == 5 || entry->type == 1) {
        if (entry->type == 1 && entry->u.text[0x80] != 0) {
            entry->u.text[0x84] = 0;
            return;
        }
        if (entry->type == 1) {
            if (strlen(entry->u.text) == 0)
                return;
            entry->u.text[0x84] = 0;
            text = entry->u.text;
        } else if (entry->type == 5) {
            entry->u.text[0x91] = 0;
            text = entry->u.text;
        }
    } else {
        return;
    }

    length = strlen(text);
    if (length == 0)
        return;

    for (i = 0; i < length; i++) {
        if (text[i] != ' ') {
            for (j = 0, scan = entries; j <= EntryCount(entries); j++, scan++) {
                if (scan->type == 1) {
                    int a = tolower((signed char)scan->u.text[0x84]);
                    int b = tolower((signed char)text[i]);
                    if (a == b)
                        break;
                } else if (scan->type == 5) {
                    int a = tolower((signed char)scan->u.text[0x91]);
                    int b = tolower((signed char)text[i]);
                    if (a == b)
                        break;
                }
            }
            if (j > EntryCount(entries)) {
                if (entry->type == 1) {
                    entry->u.text[0x84] = text[i];
                    return;
                }
                if (entry->type == 5) {
                    entry->u.text[0x91] = text[i];
                    return;
                }
                return;
            }
        }
    }
}
