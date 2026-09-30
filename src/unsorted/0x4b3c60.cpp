// Decompiled by deepseek-v4.1-flash, finished by GPT-6, finished by GPT-6.1-sol, edited by deepseek-v4.1. Names are provisional.
// Partial: 76.5%. Saved file-offset slots, loop register allocation and
// compression temporaries differ. Cached names plus += length updates and
// the vector header improve the append loops; the double record is 12 bytes.
// Retry: reordered the saved file offset without changing codegen; 128 header sets gave no improvement.
//
// deepseek-v4.1 session notes (no score gain, best stays 76.5%):
// Slot map read off the disassembly: [esp+0x10] is the append oldlen in every
// loop AND the byte offset of the count2 record loop (one slot, disjoint
// ranges), [esp+0x14] the ftell result, [esp+0x18] the saved item2->name
// pointer, [esp+0x1c]/[esp+0x20] the 8-byte and 12-byte records of the count1
// loops (rec base 0x1c), [esp+0x28]..[esp+0x37] the 16-byte count2 record,
// [esp+0x38]..[esp+0x57] the header. This file instead gets [esp+0x18] for the
// ftell result and [esp+0x14] for the count2 byte offset, i.e. one extra live
// scratch below the ftell slot, and it keeps the zero register in esi where the
// original's item loops use edi (0x4b3d13, 0x4b3deb, 0x4b3fd1 are `xor edi,edi`).
// Tried: (a) count2 loops rewritten as do/while over `(char*)slot->items2 + off`
// with the item2->len > 0 test inline, 76.1% but code 1548 bytes against the
// original 1544 (this file is 1552), so the shape was closer while the allocator
// still chose the same two slots; saved as build/scratch/0x4b3c60/a_byteoff.cpp.
// (b) one function-scope `oldlen` shared by every append and by the count2 byte
// offset, 71.2%; (c) the same with a separate `itemoff` for the count2 record
// oldlen, also 71.2%. Both spilled a live append temp, so reverting was right.
#include <vector>
#include <io.h>

struct Table_004b3630 {
    int count;
    void* slots;
};

struct Item1_004b3c60 {              // 0x10 bytes
    char* name;                      // +0x00
    int type;                        // +0x04
    int value;                       // +0x08 (int, double or char* depending on type)
    int unknown_c;                   // +0x0c
};

struct Item2_004b3c60 {              // 0x14 bytes
    int flag;                        // +0x00
    char* name;                      // +0x04 (or the id as an int when flag == 0)
    int len;                         // +0x08
    int unknown_c;                   // +0x0c
    char* buffer;                    // +0x10
};

struct Slot_004b3c60 {               // 0x18 bytes
    char* name;                      // +0x00
    int count1;                      // +0x04
    int count2;                      // +0x08
    int unknown_c;                   // +0x0c
    Item1_004b3c60* items1;          // +0x10
    Item2_004b3c60* items2;          // +0x14
};

struct Buffer_004b3c60 {             // the shared string pool
    char* data;                      // +0x00
    int len;                         // +0x04
    int csize;                       // +0x08
};

struct Header_004b3c60 {             // 0x20 bytes
    int size;                        // +0x00
    int strOffset;                   // +0x04
    int nInts;                       // +0x08
    int nDoubles;                    // +0x0c
    int nStrings;                    // +0x10
    int nBlobs;                      // +0x14
    int compressed;                  // +0x18
    int unknown_1c;                  // +0x1c
};

class Class_004b3750 {
public:
    Table_004b3630* field_0;
    char unknown_4[4];
    int field_8;

    void FUN_004b3c60(int index, FILE* file, Buffer_004b3c60* buf, int compress);
};

void* __cdecl FUN_004d8450(int size);
void* __cdecl FUN_004d8580(void* ptr, int size);
void __cdecl FUN_004d85a0(void* ptr);
int __cdecl FUN_004d8e50(int handle);
int __stdcall FUN_004d1aa0(int size, int level);
int __stdcall FUN_004d1820(void* dest, int* destSize, void* src, int srcSize, int param_5, int param_6);

// FUNCTION: 0x4b3c60
void Class_004b3750::FUN_004b3c60(int index, FILE* file, Buffer_004b3c60* buf, int compress)
{
    Slot_004b3c60* slot = &((Slot_004b3c60*)field_0->slots)[index];
    if (slot->count1 <= 0 && slot->count2 <= 0) {
        return;
    }

    int off = ftell(file);
    Header_004b3c60 h;
    memset(&h, 0, sizeof(h));
    fwrite(&h, sizeof(h), 1, file);

    {
        char* name = slot->name;
        int oldlen = buf->len;
        buf->len += strlen(name) + 1;
        buf->data = (char*)FUN_004d8580(buf->data, buf->len);
        strcpy(buf->data + oldlen, name);
        h.strOffset = oldlen;
    }

    int i;
    for (i = 0; i < slot->count1; i++) {
        if (slot->items1[i].type == 1) {
            char* name = slot->items1[i].name;
            int oldlen = buf->len;
            buf->len += strlen(name) + 1;
            buf->data = (char*)FUN_004d8580(buf->data, buf->len);
            strcpy(buf->data + oldlen, name);
            int rec[2];
            rec[0] = oldlen;
            rec[1] = slot->items1[i].value;
            fwrite(rec, 8, 1, file);
            h.nInts++;
        }
    }

    for (i = 0; i < slot->count1; i++) {
        if (slot->items1[i].type == 2) {
            char* name = slot->items1[i].name;
            int oldlen = buf->len;
            buf->len += strlen(name) + 1;
            buf->data = (char*)FUN_004d8580(buf->data, buf->len);
            strcpy(buf->data + oldlen, name);
            int rec[3];
            rec[0] = oldlen;
            *(double*)&rec[1] = *(double*)&slot->items1[i].value;
            fwrite(rec, 0xc, 1, file);
            h.nDoubles++;
        }
    }

    for (i = 0; i < slot->count1; i++) {
        if (slot->items1[i].type == 3) {
            char* name = slot->items1[i].name;
            int oldlen = buf->len;
            buf->len += strlen(name) + 1;
            buf->data = (char*)FUN_004d8580(buf->data, buf->len);
            strcpy(buf->data + oldlen, name);
            int rec[2];
            rec[0] = oldlen;
            int oldlen2 = buf->len;
            char* second = (char*)slot->items1[i].value;
            buf->len += strlen(second) + 1;
            buf->data = (char*)FUN_004d8580(buf->data, buf->len);
            strcpy(buf->data + oldlen2, second);
            rec[1] = oldlen2;
            fwrite(rec, 8, 1, file);
            h.nStrings++;
        }
    }

    for (i = 0; i < slot->count2; i++) {
        if (slot->items2[i].len > 0) {
            h.nBlobs++;
        }
    }

    int dataOffset = ftell(file) + h.nBlobs * 0x10;
    for (i = 0; i < slot->count2; i++) {
        Item2_004b3c60* it = &slot->items2[i];
        if (it->len > 0) {
            int rec[4];
            if (it->flag != 0) {
                int oldlen = buf->len;
                buf->len += strlen(it->name) + 1;
                buf->data = (char*)FUN_004d8580(buf->data, buf->len);
                strcpy(buf->data + oldlen, it->name);
                rec[0] = oldlen;
            } else {
                rec[0] = -1;
                rec[1] = (int)it->name;
            }
            rec[2] = dataOffset;
            rec[3] = it->len;
            fwrite(rec, 0x10, 1, file);
            dataOffset += it->len;
        }
    }

    for (i = 0; i < slot->count2; i++) {
        Item2_004b3c60* it = &slot->items2[i];
        if (it->len > 0) {
            fwrite(it->buffer, it->len, 1, file);
        }
    }

    h.size = ftell(file) - off;

    if (compress != 0) {
        int handle = FUN_004d8e50(0);
        int len = h.size - 0x20;
        char* raw = (char*)FUN_004d8450(len);
        if (raw != 0) {
            fseek(file, off + 0x20, 0);
            fread(raw, len, 1, file);
            int csize = FUN_004d1aa0(len, 1);
            char* cbuf = (char*)FUN_004d8450(csize);
            if (cbuf != 0) {
                int err = FUN_004d1820(cbuf, &csize, raw, len, 1, 0);
                if (err == 0 && csize < len) {
                    fseek(file, off + 0x20, 0);
                    fwrite(cbuf, csize, 1, file);
                    h.size = csize + 0x20;
                    h.compressed = 1;
                    _chsize(file->_file, ftell(file));
                }
                FUN_004d85a0(cbuf);
            }
            FUN_004d85a0(raw);
        }
        FUN_004d8e50(handle);
    }

    fseek(file, off, 0);
    fwrite(&h, sizeof(h), 1, file);
    fseek(file, 0, 2);
}
