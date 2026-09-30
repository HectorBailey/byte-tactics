// Decompiled by GPT-6, finished by space-bunny-free, edited by deepseek-v4.1. Names are provisional.
// Credit: started by GPT-6, continued by space-bunny-free (left at 94.8%).
// Partial: 94.8%. The only non-relocation difference is the order of two
// instructions at 0x4ae7cc: the original emits `mov ebx, 2` and only then
// `mov byte ptr [esp + 0x13], 9`; ours emits the store first.
// Third pass tried (all came back 94.5-94.8%, or 91.5% for the helper):
//   i = 2; tab = '\t'; do {...} while (--i)        -> store first
//   tab = '\t'; i = 2; do {...} while (--i)        -> store first
//   tab = '\t'; for (i = 0; i < 2; i++) call       -> store first
//   int k = 2; tab = '\t'; do {...} while (--k)    -> store first
//   i = 2, tab = '\t'; (comma expression)          -> store first
//   for (i = 2, tab = '\t'; i; i--) call           -> store first
//   i = 2; do { tab = '\t'; call; } while (--i)    -> store first, and the
//       in-body store gets its own home at [esp+0x1f] (94.5%)
//   i = 2; do { call; tab = '\t'; } while (--i)    -> `mov ebx, 2` first,
//       but the store is hoisted to after the loop (94.5%)
//   static helper WriteTabs(out, n) inlined at the site -> 91.5%: inlining
//       reallocates the locals (the buffer at [esp+0xe0] moves to
//       [esp+0x144]), so the frame layout stops matching even though a
//       helper body would explain the argument-materialised-first order.
// Conclusion: MSVC 5 sinks the loop counter's `mov ebx, 2` past a store that
// precedes the loop, and every spelling that keeps the store in the loop body
// either gives it a temp home or hoists it after the loop. Remaining hunks:
// 0x4ae7cc-0x4ae7d1 (this order) and 0x4aea8c (jump table, relocation only).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Class_004bbbe0;
char* __stdcall FUN_004baff0(char*, char*, const char*);
int __stdcall FUN_004bbc40(char*);
void __stdcall FUN_004bbc30(char*);
void __stdcall FUN_004bbc10(char*, char*);
Class_004bbbe0* __stdcall FUN_004bb6a0(char*);
void __stdcall FUN_004bb5d0(Class_004bbbe0*);
unsigned int __stdcall FUN_004bbbe0(Class_004bbbe0*, void*, unsigned int);
void __stdcall FUN_004accd0(Class_004bbbe0*, int);
void __stdcall FUN_004acde0(Class_004bbbe0*, char*, char*, int);
void __stdcall FUN_004ace50(void*, Class_004bbbe0*, int);
void __stdcall FUN_004ad4f0(void*, Class_004bbbe0*, int);

// FUNCTION: 0x4ae630
void __stdcall FUN_004ae630(char* obj, char* name)
{
    char tab;
    int index;
    char button[100];
    char slider[100];
    char header[100];
    char common[100];
    char gadget[100];
    char path[256];
    char backup[256];
    char hot[100];
    char edit[100];
    char empty[100];
    char list[100];
    FUN_004baff0(name, path, "GUI");
    if (FUN_004bbc40(path)) {
        FUN_004baff0(name, backup, "BGU");
        FUN_004bbc30(backup);
        FUN_004bbc10(path, backup);
    }
    Class_004bbbe0* out = FUN_004bb6a0(path);
    char* p = obj;
    for (index = 0; index < *(short*)(obj + 0xb6) + 1; index++, p += 0x15b) {
        sprintf(gadget, "GADGET%d", index);
        sprintf(header, "[%s]", gadget);
        FUN_004bbbe0(out, header, strlen(header));
        FUN_004bbbe0(out, "\n", 1);
        FUN_004accd0(out, 1);
        FUN_004bbbe0(out, "{\n", 2);
        sprintf(common, "[%s]", "COMMON");
        tab = '\t';
        for (int i = 0; i < 1; i++) FUN_004bbbe0(out, &tab, 1);
        FUN_004bbbe0(out, common, strlen(common));
        FUN_004bbbe0(out, "\n", 1);
        FUN_004accd0(out, 2);
        FUN_004bbbe0(out, "{\n", 2);
        FUN_004ace50(p, out, 2);
        i = 2;
        tab = '\t';
        do { FUN_004bbbe0(out, &tab, 1); } while (--i);
        FUN_004bbbe0(out, "}\n", 2);
        switch (*(unsigned char*)p) {
        case 0:
            FUN_004ad4f0(p, out, 1);
            break;
        case 1:
            FUN_004acde0(out, "status", _itoa(*(short*)(p + 0x138), button, 10), 1);
            FUN_004acde0(out, "text", p + 0xb6, 1);
            FUN_004acde0(out, "quickkey", _itoa(*(signed char*)(p + 0x13a), button, 10), 1);
            FUN_004acde0(out, "grayedout", _itoa(*(unsigned char*)(p + 0x13c) & 1, button, 10), 1);
            FUN_004acde0(out, "stages", _itoa(*(unsigned char*)(p + 0x136), button, 10), 1);
            break;
        case 2:
            FUN_004acde0(out, "itemheight", _itoa(*(short*)(p + 0xda), list, 10), 1);
            break;
        case 3:
            FUN_004acde0(out, "maxchars", _itoa(*(short*)(p + 0x138), edit, 10), 1);
            FUN_004acde0(out, "text", p + 0xb6, 1);
            break;
        case 4:
            FUN_004acde0(out, "range", _itoa(*(short*)(p + 0x136), slider, 10), 1);
            FUN_004acde0(out, "thick", _itoa(*(int*)(p + 0x13c), slider, 10), 1);
            FUN_004acde0(out, "knobpos", _itoa(*(short*)(p + 0x140), slider, 10), 1);
            FUN_004acde0(out, "knobsize", _itoa(*(short*)(p + 0x142), slider, 10), 1);
            break;
        case 5:
            FUN_004acde0(out, "text", p + 0xb6, 1);
            FUN_004acde0(out, "link", p + 0x136, 1);
            break;
        case 6:
            FUN_004acde0(out, "hotornot", _itoa(*(unsigned int*)(p + 0xc8) & 1, hot, 10), 1);
            break;
        case 7:
            FUN_004acde0(out, "filename", p + 0xb6, 1);
            break;
        case 8:
            FUN_004acde0(out, "filename", p + 0xb6, 1);
            break;
        case 10:
            FUN_004acde0(out, "nuttin", _itoa(*(int*)(p + 0xb6), empty, 10), 1);
            break;
        }
        tab = '\t';
        for (i = 0; i < 1; i++) FUN_004bbbe0(out, &tab, 1);
        FUN_004bbbe0(out, "}\n", 2);
    }
    FUN_004bb5d0(out);
}
