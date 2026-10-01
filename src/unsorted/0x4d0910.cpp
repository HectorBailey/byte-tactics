// Decompiled by deepseek-v4.1-flash. Names are provisional.
// 93.3 percent, best attempt. This is the same chunk-scanner as the FindChunk
// helper inlined in 0x4d02a0 (see that file), specialized to the "data" tag.
// The only remaining diff is `limit += 8`: the original emits the register form
// `mov edx,[esp+8]; add edx,8; mov [esp+0x10],edx` (a copy-with-offset, load and
// store split and interleaved with the pushes of the next call), while this code
// emits the shorter memory form `add dword ptr [esp+0x10],8`, so ours is 200
// bytes vs the original 206. Tried `limit = limit + 8` and `limit += 8`, both
// fold to the memory add. Likely needs the value read into one variable then
// copied+8 into a coalesced sibling to force the register form; that variant was
// not scored before the timebox ended.
#include <string.h>

struct File_004bb5d0;

int __stdcall FUN_004bb710(File_004bb5d0* file, int pos);
int __stdcall FUN_004bb7c0(File_004bb5d0* file, void* buf, int size);

// FUNCTION: 0x4d0910
unsigned int FUN_004d0910(File_004bb5d0* file) {
    unsigned int limit, id, offset, size;
    FUN_004bb710(file, 4);
    FUN_004bb7c0(file, &limit, 4);
    limit += 8;
    FUN_004bb710(file, 12);
    FUN_004bb7c0(file, &id, 4);
    FUN_004bb7c0(file, &size, 4);
    offset = 20;
    for (;;) {
        if (strncmp((char*)&id, "data", 4) == 0)
            return size;
        FUN_004bb710(file, size + offset);
        offset += size;
        if (offset >= limit)
            return 0;
        FUN_004bb7c0(file, &id, 4);
        FUN_004bb7c0(file, &size, 4);
        offset += 8;
    }
}
