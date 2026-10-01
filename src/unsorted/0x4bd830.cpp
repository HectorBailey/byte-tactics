// Decompiled by DeepSeek V4.1 Flash, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro. Names are provisional.
// Session 13 (mimo-v2.6-pro): best is now 64.7 percent (this file, 1353 bytes).
// Fixed on top of the 61.9 file: the two `file->shared == 0` tests flipped to
// `!= 0` with swapped arms (both the size if-chain and the close logic, which
// also moves the fclose(fp) block to the end as in the original), the size
// if-chain written as else-if, `unsigned clen` and both `unsigned chunk` made
// int (the obfuscation loops compare signed: jle/jl in the original), and both
// min-selects respelled per the guide's 0x4dba40 note: `x >= K ? K : x` gives
// `mov reg, K; jae; mov reg, x`, which is exactly the original's shape (the
// old `x < K ? x : K` spelling precomputed the other arm). What still differs:
// (1) the register mirror, base in ebx and the entry pointer in ebp here vs
// base in ebp and entry pointer in ebx in the original; (2) the stack-slot map
// is still a permutation (ours nameoff +0x28, i +0x38, count +0x40; original
// nameoff +0x20, i +0x34, recarr +0x3c with no count slot at all and packlen
// +0x40); (3) the nblocks block is signed now (`int nblocks(int w)`, worth
// +2.5 points on top of this file; the original spelled
// `int blocks = size / 65536 + (size % 65536 != 0)` with signed division) but
// size sits in ebp here where the original computes it from ecx; (4) the entry
// loop latch re-derefs *(unsigned*)(base + off) inline in the original while
// this file keeps a count local (dropping it shrinks the frame and scores far
// worse on its own); (5) the two obfuscation loops spill pos to a slot and
// keep key in a byte register across iterations, this file reloads key each
// iteration; the original reuses table's slot for the table-obfuscation pos so
// the second pos is a separate local; (6) the callback computes the subtracted
// *(int*)(base + 8) * 90 term first into ecx, this file interleaves both
// multiply chains.
#include <stdio.h>
#include <string.h>
#include <io.h>

#pragma pack(push, 1)
struct Node_004bd830 {
    char unknown_0[0xc];
    unsigned char obfuscate;             // +0xc
};

struct Item_004bd830 {
    FILE* fp;                            // +0x0
    int pos;                             // +0x4
    Node_004bd830* node;                 // +0x8
    int count;                           // +0xc
    int unknown_10;                      // +0x10
    char name[0x100];                    // +0x14
};

struct Info_004bd830 {
    int offset;                          // +0x0
    int size;                            // +0x4
    unsigned char compressed;            // +0x8
};

struct File_004bd830 {
    FILE* fp;                            // +0x0
    Item_004bd830* shared;               // +0x4
    Info_004bd830* info;                 // +0x8
    unsigned int pos;                    // +0xc
    int* buffer;                         // +0x10
    unsigned char* buffer2;              // +0x14
    char name[0x100];                    // +0x18
};

struct Entry_004bd830 {
    int name;                            // +0x0, offset of the name string
    int offset;                          // +0x4, offset of the record data
    unsigned char flags;                 // +0x8
};
#pragma pack(pop)

File_004bd830* __stdcall FUN_004bb2e0(char* filename, const char* mode);
int __stdcall FUN_004bb7c0(File_004bd830* file, unsigned char* buf, int size);
int __stdcall FUN_004d1820(void* chunk, int* chunkSize, char* data,
                           int size, int method, int encrypt);
unsigned int __stdcall FUN_004d1aa0(unsigned int value, int mode);
void* __cdecl FUN_004d83b0(char* name, unsigned int size);
void __cdecl FUN_004d85a0(void* p);
void __stdcall FUN_004bd830(char* path, char* base, int off, FILE* f,
                            void (__cdecl* cb)(unsigned), unsigned extra,
                            int key, int flags);

// Whole 64K blocks of a byte size, rounded up.
static inline int nblocks(int w)
{
    return w / 65536 + (w % 65536 != 0);
}

// FUNCTION: 0x4bd830
void __stdcall FUN_004bd830(char* path, char* base, int off, FILE* f,
                            void (__cdecl* cb)(unsigned), unsigned extra,
                            int key, int flags)
{
    char name[260];
    char full[260];
    unsigned char buffer[0x1000];
    long* dataptr;
    unsigned remaining;
    unsigned char* data;
    int* tp;
    int nameoff;
    int* table;
    int clen;
    File_004bd830* file;
    int n;
    unsigned i;
    long pos;
    int* recoff;
    unsigned packlen;

    strcpy(name, path);
    if (name[strlen(name) - 1] != '\\')
        strcat(name, "\\");

    unsigned count = *(unsigned*)(base + off);
    i = 0;
    if (i < count) {
        nameoff = 0;
        recoff = (int*)(base + off + 4);
        do {
            Entry_004bd830* e = (Entry_004bd830*)(base + nameoff + *recoff);
            strcpy(full, name);
            strcat(full, (char*)(base + e->name));
            if ((e->flags & 1) != 0) {
                FUN_004bd830(full, base, e->offset, f, cb, extra, key, flags);
            } else {
                file = FUN_004bb2e0(full, "rb");
                dataptr = (long*)(base + e->offset);
                *dataptr = ftell(f);
                unsigned size;
                if (file->shared != 0) {
                    size = file->info->size;
                } else if (file->fp != 0) {
                    size = _filelength(_fileno(file->fp));
                } else {
                    size = 0;
                }
                *(unsigned*)(dataptr + 1) = size;
                *((char*)dataptr + 8) = (char)flags;
                if ((char)flags != 0) {
                    int blocks = nblocks(size);
                    table = (int*)FUN_004d83b0("Block Sizes", blocks * 4);
                    fwrite(table, blocks, 4, f);
                    packlen = FUN_004d1aa0(0x10000, flags & 0xff);
                    unsigned char* pack = (unsigned char*)FUN_004d83b0("Pack Buffer", packlen);
                    data = (unsigned char*)FUN_004d83b0("Data Buffer", 0x10000);
                    remaining = size;
                    n = blocks;
                    tp = table;
                    if (blocks > 0) {
                        do {
                            int chunk = remaining >= 0x10000 ? 0x10000 : remaining;
                            FUN_004bb7c0(file, data, chunk);
                            clen = packlen;
                            FUN_004d1820(pack, &clen, (char*)data, chunk, flags & 0xff, 1);
                            *tp = clen;
                            pos = ftell(f);
                            if ((char)key != 0) {
                                for (int j = 0; j < clen; j++)
                                    pack[j] = (unsigned char)~((char)pos + (char)j
                                              ^ (char)key ^ pack[j]);
                            }
                            fwrite(pack, clen, 1, f);
                            tp++;
                            remaining -= 0x10000;
                            n--;
                        } while (n != 0);
                    }
                    fseek(f, *dataptr, 0);
                    pos = ftell(f);
                    if ((char)key != 0) {
                        for (int j = 0; j < (int)(blocks * 4); j++)
                            ((unsigned char*)table)[j] = (unsigned char)~((char)pos
                                + (char)j ^ (char)key ^ ((unsigned char*)table)[j]);
                    }
                    fwrite(table, blocks, 4, f);
                    fseek(f, 0, 2);
                    FUN_004d85a0(table);
                    FUN_004d85a0(pack);
                    FUN_004d85a0(data);
} else {
                    if (size > 0) do {
                        int chunk = size >= 0x1000 ? 0x1000 : size;
                        FUN_004bb7c0(file, buffer, chunk);
                        pos = ftell(f);
                        if ((char)key != 0) {
                            for (int j = 0; j < chunk; j++)
                                buffer[j] = (unsigned char)~((char)pos + (char)j
                                            ^ (char)key ^ buffer[j]);
                        }
                        fwrite(buffer, chunk, 1, f);
                        size -= chunk;
                    } while (size != 0);
}
                if (file->shared != 0) {
                    file->shared->count--;
                    if (file->shared->count == 0 && file->shared->unknown_10 == 0) {
                        fclose(file->shared->fp);
                        file->shared->fp = 0;
                    }
                } else {
                    fclose(file->fp);
                }
                if (file->buffer != 0)
                    FUN_004d85a0(file->buffer);
                if (file->buffer2 != 0)
                    FUN_004d85a0(file->buffer2);
                FUN_004d85a0(file);
                if (cb != 0)
                    cb((unsigned)(*dataptr * 90 - *(int*)(base + 8) * 90) / extra + 5);
            }
            nameoff += 9;
            i++;
        } while (i < count);
    }
}
