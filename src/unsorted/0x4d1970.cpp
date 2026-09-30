// Decompiled by deepseek-v4.1. Names are provisional.
// Unpacks a "SQSH" chunk (see FUN_004d1820 for the packer).
// Partial at 71.2% (304 bytes vs 296). What still differs: the prologue and
// every register that follows it. The original loads the data pointer before
// the register saves and keeps it in edx through the 19-byte header copy
// (sub esp,0x14 / mov edx,[esp+0x1c] / push ebx / mov eax,edx / push ebp ...),
// so its copy temp is ecx and the byte tail is al, and the payload pointer is
// `add edx,0x13`. Ours loads the pointer into ebx after `push ebx` and uses
// edx as the copy temp (dl for the byte), which shifts every later register
// and the call setup. Also the type==2 arm: the original keeps the
// memcmp(data,"SQSH",4) ? *(int*)(data+0xb) : 0 value in eax and stores it to
// the destLen slot during the argument setup; ours stores 0 or the value in
// each arm of the branch. Tried and all identical (71.2%, 304 bytes): plain
// memcpy vs struct assignment, a char* alias for the memcpy source, adding
// <windows.h>/<stdio.h>/<stdlib.h> to the includes.
#include <string.h>

#pragma pack(push, 1)
struct Chunk_4d1970 {
    int marker;                      // +0x00 "SQSH"
    unsigned char version;           // +0x04
    unsigned char method;            // +0x05
    unsigned char encrypt;           // +0x06
    int compressedSize;              // +0x07
    int size;                        // +0x0b
    int checksum;                    // +0x0f
};
#pragma pack(pop)

int __stdcall FUN_004d1480(unsigned char* dest, unsigned char* src);
int __stdcall _uncompress(unsigned char* dest, unsigned long* destLen, unsigned char* source, unsigned long sourceLen);

// FUNCTION: 0x4d1970
int __stdcall FUN_004d1970(char* dest, char* data)
{
    Chunk_4d1970 header;
    memcpy(&header, data, 0x13);
    if (memcmp(&header, "SQSH", 4) != 0) {
        return 1;
    }
    if (header.method >= 4) {
        return 4;
    }
    int sum = 0;
    for (unsigned char* p = (unsigned char*)(data + 0x13); p < (unsigned char*)(data + 0x13) + header.compressedSize; p++) {
        sum += *p;
    }
    if (header.checksum != sum) {
        return 2;
    }
    if (header.encrypt) {
        for (unsigned int i = 0; i < header.compressedSize; i++) {
            data[0x13 + i] = (data[0x13 + i] - i) ^ i;
        }
    }
    int length;
    switch (header.method) {
    case 1:
        length = FUN_004d1480((unsigned char*)dest, (unsigned char*)(data + 0x13));
        break;
    case 2:
        length = memcmp(data, "SQSH", 4) == 0 ? *(int*)(data + 0xb) : 0;
        _uncompress((unsigned char*)dest, (unsigned long*)&length, (unsigned char*)(data + 0x13), header.compressedSize);
        break;
    }
    return (header.size - length) ? 3 : 0;
}
