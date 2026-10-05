// Decompiled by GPT-6, finished by space-bunny-free, edited by deepseek-v4.1. Names are provisional.
// Credit: started by GPT-6, continued by space-bunny-free (left at 94.8%).
// MATCH. The last hunk was the two-tab loop, where the original emits
// `mov ebx, 2` before `mov byte ptr [esp + 0x13], 9`; the cause is a
// block-scope definition for both the counter and the char (`int j = 2;`
// then `char t = '\t';` in one block), which MSVC 5 emits in source order.
// Loops 1 and 3 must keep the plain `tab = '\t';` plus `for` spelling: an
// assignment sinks behind the counter and stores first, which is what the
// original does there. The three block-scope chars (t1, t2, t3) share the
// one slot at [esp+0x13], so the frame is unchanged.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Class_004bbbe0;
char* __stdcall ChangeExtension(char*, char*, const char*);
int __stdcall HAPI_FileLengthByName(char*);
void __stdcall RemoveFile(char*);
void __stdcall RenameFile(char*, char*);
Class_004bbbe0* __stdcall HAPI_CreateFile(char*);
void __stdcall HAPI_CloseFile(Class_004bbbe0*);
unsigned int __stdcall HAPI_WriteFile(Class_004bbbe0*, void*, unsigned int);
void __stdcall FUN_004accd0(Class_004bbbe0*, int);
void __stdcall FUN_004acde0(Class_004bbbe0*, char*, char*, int);
void __stdcall FUN_004ace50(void*, Class_004bbbe0*, int);
void __stdcall FUN_004ad4f0(void*, Class_004bbbe0*, int);

// FUNCTION: 0x4ae630
void __stdcall FUN_004ae630(char* obj, char* name)
{
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
    ChangeExtension(name, path, "GUI");
    if (HAPI_FileLengthByName(path)) {
        ChangeExtension(name, backup, "BGU");
        RemoveFile(backup);
        RenameFile(path, backup);
    }
    Class_004bbbe0* out = HAPI_CreateFile(path);
    char* p = obj;
    for (index = 0; index < *(short*)(obj + 0xb6) + 1; index++, p += 0x15b) {
        sprintf(gadget, "GADGET%d", index);
        sprintf(header, "[%s]", gadget);
        HAPI_WriteFile(out, header, strlen(header));
        HAPI_WriteFile(out, "\n", 1);
        FUN_004accd0(out, 1);
        HAPI_WriteFile(out, "{\n", 2);
        sprintf(common, "[%s]", "COMMON");
        {
            char t1 = '\t';
            for (int i = 0; i < 1; i++) HAPI_WriteFile(out, &t1, 1);
        }
        HAPI_WriteFile(out, common, strlen(common));
        HAPI_WriteFile(out, "\n", 1);
        FUN_004accd0(out, 2);
        HAPI_WriteFile(out, "{\n", 2);
        FUN_004ace50(p, out, 2);
        {
            int j = 2;
            char t2 = '\t';
            do { HAPI_WriteFile(out, &t2, 1); } while (--j);
        }
        HAPI_WriteFile(out, "}\n", 2);
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
        {
            char t3 = '\t';
            for (int i = 0; i < 1; i++) HAPI_WriteFile(out, &t3, 1);
        }
        HAPI_WriteFile(out, "}\n", 2);
    }
    HAPI_CloseFile(out);
}
