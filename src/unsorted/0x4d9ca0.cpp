// Decompiled by space-bunny-free. Names are provisional.
// NOT A MATCH: 67.2% at the time of writing (1 real check.py run).
// The code is right, the register allocation is not. The original keeps SIX
// memory-resident locals (esp+0x10 len, +0x14 copied, +0x18 the walking
// pointer, +0x1c count/stack pointer, +0x20 this, +0x24 pc) and so never
// needs a frame pointer: ebp is the loop counter and ebx is the buffer
// pointer. Copying the member values into named locals (v2 in
// build/scratch/0x4d9ca0/v2.cpp) makes it far worse, 30.1%, so the locals
// are not the problem; what is missing is a source construct that makes
// MSVC 5 spill all of them at once instead of promoting `this` to ebp.
// Second difference: the original's `abs(abs(i) & 7)` is one `cdq` shorter
// than mine in both loops, i.e. it reuses the sign already in edx rather
// than recomputing it, so the second absolute value is not a source-level
// abs() of the masked value.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class Class_004d9ca0 {
public:
    unsigned long ret[0x1e];           // +0x000: recorded return addresses
    int count;                         // +0x078: how many of them
    int stack[0x800];                  // +0x07c: copy of the raw stack
    int copied;                        // +0x207c: dwords copied above
    unsigned long* pc;                 // +0x2080: current program counter
    char buf[0xa44c];                  // +0x2084: the dump buffer

    void FUN_004d9ca0();
};

// FUNCTION: 0x4d9ca0
void Class_004d9ca0::FUN_004d9ca0()
{
    char* p = buf;
    int len = 0xa44c;
    if (count > 0) {
        sprintf(p, "Call stack:\n");
        len -= (int)strlen(p);
        p += strlen(p);
        for (int i = 0; i < count; i++) {
            if (len <= 0x1e)
                break;
            sprintf(p, "%08lX", ret[i]);
            strcat(p, (i == count - 1 || abs(abs(i) & 7) == 7) ? "\n" : " ");
            len -= (int)strlen(p);
            p += strlen(p);
        }
    } else {
        p[0] = 0;
    }
    if (copied > 0 && len > 0x1e) {
        sprintf(p, "Stack dump:\n");
        len -= (int)strlen(p);
        p += strlen(p);
        unsigned long* pcv = pc;
        int* sp = stack;
        for (int i = 0; i < copied; i++) {
            if (len <= 0x1e)
                break;
            if (abs(abs(i) & 7) == 0) {
                sprintf(p, "%08lX: ", (unsigned long)pcv);
                len -= (int)strlen(p);
                p += strlen(p);
            }
            sprintf(p, "%08lX", *sp);
            strcat(p, (i == copied - 1 || abs(abs(i) & 7) == 7) ? "\n" : " ");
            len -= (int)strlen(p);
            p += strlen(p);
            pcv += 4;
            sp++;
        }
    }
}
