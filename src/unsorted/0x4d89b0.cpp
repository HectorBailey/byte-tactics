// Decompiled by deepseek-v4.1. Names are provisional.
// Partial 92.9% (295 bytes vs 293). Builds a "name(line) : <call stack>" debug string.
// Still differs: only the FUN_004dea00 argument block. Original order is
//   strlen #1, add ebp,0x44, push ebp (arg5), strlen #2, size load, sub,
//   mov ecx,[ebp+0x38], push ecx (arg4), push 0xe, push eax, push edx.
// Ours loads s.count into eax and pushes it right after arg5, before the
// second strlen; because that clobbers eax we emit a second `xor eax,eax`
// (the 2 extra bytes), the original reuses al==0 from the first scan.
// Tried and all 92.1-92.9: plain field_44/field_7c args, nested struct
// without reference, unsigned long* local, (unsigned long*)&stack cast,
// pointer instead of reference (92.9, same code), reference declared after
// q (92.9), count direct on the class with s.addrs (92.1).
#include <stdio.h>
#include <string.h>

struct Stack_004d89b0 {
    unsigned long addrs[14];    // +0x00
    int count;                  // +0x38
};

class Class_004d89b0 {
public:
    char name[0x40];            // +0x00
    int field_40;               // +0x40
    Stack_004d89b0 stack;       // +0x44
    void FUN_004d89b0(char* out, int size);
};

void __cdecl FUN_004de8a0(char* out, char* name);
void __cdecl FUN_004dea00(char* dest, int space, int per, int n, unsigned long* addrs);

// FUNCTION: 0x4d89b0
void Class_004d89b0::FUN_004d89b0(char* out, int size)
{
    char path[1000];

    out[0] = 0;
    strcpy(out, "\n");
    if (name[0]) {
        FUN_004de8a0(path, name);
        char* p = out + strlen(out);
        sprintf(p, "%s(%d)", path, field_40);
        strcat(out, " : ");
        strcat(out, "\n");
    }
    Stack_004d89b0& s = stack;
    char* q = out + strlen(out);
    FUN_004dea00(q, size - strlen(out), 14, s.count, s.addrs);
    int len = strlen(out);
    if (len != 0 && out[len - 1] == '\n')
        out[len - 1] = 0;
}
