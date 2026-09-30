// Decompiled by Sonnet 5.5, finished by deepseek-v4.1-flash, GPT-6, GPT-6.1-sol, and deepseek-v4.1. Names are provisional.
// Retry #1964: deepseek-v4.1, still 99.2% (1042 bytes, exact size). One diff left, a pure
// register choice: the reload of the clamped size n at 0x4bb807 (feeds remaining = n and
// the block-end lea) lands in esi here (mov esi,[esp+0x14]; mov [esp+0x1c],esi;
// lea esi,[esi+eax-1]) but in ebx in the original (mov ebx,[esp+0x14];
// mov [esp+0x1c],ebx; lea esi,[ebx+eax-1]).
// The 98.6% hunk is fixed by writing int off = file->pos + file->info->offset; (through
// file->info, not the cached info local): that is the only form that puts off in eax
// plus a copy to ebx instead of loading [esi] into ebx first.
// New measurements this pass (all variants compiled and scored directly, see
// build/scratch/0x4bb7c0/sweep*.py): the register of that reload is driven by which
// statement uses n first: remaining = n first gives esi, blocks first gives edx, and
// tableSize first gives ebx. The tableSize-first form (tableSize, then blocks) is the
// only spelling found that produces the original ebx, but it re-colours the whole
// function (info moves to edx, info->size to ecx, the frame stores reorder) and drops to
// 78.6%, so the original's source order is the current one and the ebx pick is
// allocator state, not statement order. Also tried and rejected, all 99.2% or worse:
// comma forms (remaining = (i = 0, n)), (void)n, n = n, an inlined identity helper
// around n in the store and in the blocks expression, a reference identity, if (n) {},
// *(int*)&n, clamp as a ternary (98.6%), the reversed clamp, int avail, 24 permutations
// of remaining / i / blocks / tableSize, for (; i < blocks; ++i), do/while(0) around the
// compressed body, a goto-plain form of the compressed test, unsigned/long n and
// remaining, splitting the blocks expression into two statements, and all 128 header
// sets from tools/headers.py (every set 99.2%, 72 failed to compile).
// The block-count/table-size order stays load bearing: writing tableSize as
// ((size % 65536 != 0) + size / 65536) * 4 (modulo first) makes blocks land in
// esi and tableSize in edi as the original does.
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)
struct Node_004bb7c0 {
    char unknown_0[0xc];
    unsigned char obfuscate;             // +0xc
};

struct Item_004bb7c0 {
    FILE* fp;                            // +0x0
    int pos;                             // +0x4
    Node_004bb7c0* node;                 // +0x8
    int count;                           // +0xc
    char unknown_10[4];
    char name[0x100];                    // +0x14
};

struct Info_004bb7c0 {
    int offset;                          // +0x0
    int size;                            // +0x4
    unsigned char compressed;            // +0x8
};

struct File_004bb7c0 {
    FILE* fp;                            // +0x0
    Item_004bb7c0* shared;               // +0x4
    Info_004bb7c0* info;                 // +0x8
    unsigned int pos;                    // +0xc
    int* buffer;                         // +0x10, block sizes
    unsigned char* buffer2;              // +0x14, the current block
    char name[0x100];                    // +0x18
};
#pragma pack(pop)

void* FUN_004d83b0(char* name, unsigned int size);
void FUN_004d85a0(void* p);
long __stdcall FUN_004bb710(File_004bb7c0* file, long pos);
int __stdcall FUN_004d1970(unsigned char* dst, unsigned char* src);
char* __stdcall FUN_004d1c60(int code);
void __stdcall FUN_004b6290(char* text);

static inline int nblocks(int w)
{
    return w / 65536 + (w % 65536 != 0);
}

static inline int BlockOffset(File_004bb7c0* file, int& counter, int b, int tableSize)
{
    int off = tableSize;
    for (counter = 0; counter < b; ++counter)
        off += file->buffer[counter];
    return off;
}
// FUNCTION: 0x4bb7c0
int __stdcall FUN_004bb7c0(File_004bb7c0* file, unsigned char* buf, int size)
{
    int n;
    unsigned char* dst;
    int remaining;
    unsigned char* comp;
    int i;
    int blocks;
    int tableSize;
    int b;
    Item_004bb7c0* item = file->shared;
    if (item != 0) {
        Info_004bb7c0* info = file->info;
        n = size;
        if (info->size - (int)file->pos < n)
            n = info->size - (int)file->pos;
        if (info->compressed != 0) {
            remaining = n;
            i = 0;
            blocks = (((n + file->pos - 1) & 0xffff0000) - (file->pos & 0xffff0000) >> 16) + 1;
            tableSize = ((info->size % 65536 != 0) + info->size / 65536) * 4;
            dst = buf;
            while (i < blocks) {
                b = file->pos >> 16;
                if (file->buffer2 == 0) {
                    int k;
                    int off = BlockOffset(file,k,b,tableSize);
                    off += file->info->offset;
                    fseek(file->shared->fp, off, 0);
                    file->shared->pos = off;
                    file->buffer2 = (unsigned char*)FUN_004d83b0("Uncompressed Block", 0x10000);
                    comp = (unsigned char*)FUN_004d83b0("Compressed Buffer", file->buffer[k]);
                    int got = fread(comp, 1, file->buffer[k], file->shared->fp);
                    if (got != file->buffer[k]) {
                        FUN_004d85a0(file->buffer2);
                        file->buffer2 = 0;
                        FUN_004d85a0(comp);
                        return -1;
                    }
                    file->shared->pos += got;
                    unsigned char key = file->shared->node->obfuscate;
                    if (key) {
                        for (int k = 0; k < got; k++)
                            comp[k] = (unsigned char)(comp[k] ^ 0xff ^ (unsigned char)(k + off) ^ key);
                    }
                    int err = FUN_004d1970(file->buffer2, comp);
                    if (err) {
                        char msg[1000];
                        sprintf(msg, "[HAPI_readfromfile] Decompression Error: %s\n", FUN_004d1c60(err));
                        sprintf(msg + strlen(msg), "block %d of %d\n", i, blocks);
                        sprintf(msg + strlen(msg), "base name '%s'\n", file->shared->name);
                        sprintf(msg + strlen(msg), "length = %d\n", size);
                        sprintf(msg + strlen(msg), "name = '%s'\n", file->name);
                        FUN_004b6290(msg);
                    }
                    FUN_004d85a0(comp);
                }
                int chunk = ((b + 1) << 16) - file->pos;
                if (chunk > remaining)
                    chunk = remaining;
                int o = file->pos & 0xffff;
                if (chunk == 1)
                    *dst = file->buffer2[o];
                else
                    memcpy(dst, file->buffer2 + o, chunk);
                dst += chunk;
                FUN_004bb710(file, file->pos + chunk);
                remaining -= chunk;
                i++;
            }
            goto done;
        }
        int off = file->pos + file->info->offset;
        if (off != item->pos) {
            fseek(item->fp, off, 0);
            file->shared->pos = off;
        }
        n = fread(buf, 1, n, file->shared->fp);
        unsigned char key = file->shared->node->obfuscate;
        if (key) {
            for (int i = 0; i < n; i++)
                buf[i] = (unsigned char)(buf[i] ^ 0xff ^ (unsigned char)(i + off) ^ key);
        }
        if (n < 0)
            goto done;
        file->pos += n;
        file->shared->pos += n;
        goto done;
    }
    n = fread(buf, 1, size, file->fp);
done:
    return n;
}
