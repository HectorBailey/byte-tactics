// Decompiled by Sonnet 5.5. Names are provisional.
// Walks a directory tree (the search 0x4bc4b0 allocates, the same one 0x4bcb50
// uses) and, for every plain file, marks the matching entry of every open
// HAPI archive: the entry named "path + file name" is looked up with
// FUN_004bb4e0 in each archive's directory (from the search's current archive
// index on) and gets bit 2 set unless bit 1 is already set. Sub directories
// other than "." and ".." are entered with the path extended by "name\\".
//
// NOT MATCHED: 99.2%, size exact. One branch target differs: when the file
// loop has nothing to do (`i >= count` on entry) the original jumps past the
// reload of the search handle into esi (it only needs reloading after the loop
// body has used esi); here the guard jumps to the reload. The `for`/`while`
// spelling, a pointer-typed handle and the local order change nothing.
#include <io.h>
#include <string.h>

#pragma pack(push, 1)
struct Find_004be400 {
    char dir[0x100];         // +0x000
    char pattern[0x100];     // +0x100
    int state;               // +0x200
    char recursive;          // +0x204
    long handle;             // +0x205
    int index;               // +0x209
};

struct Entry_004be400 {                // 9 bytes
    int field_0;
    int field_4;
    unsigned char flags;               // +0x8
};

struct Table_004be400 {
    int count;
    Entry_004be400* entries;
};

struct Header_004be400 {
    char unknown_0[0x10];
    Table_004be400* table;             // +0x10
};

struct File_004be400 {
    char unknown_0[8];
    Header_004be400* header;           // +0x8
};

struct Display_004be400 {
    char unknown_0[0x618];
    File_004be400** files;             // +0x618
    int count;                         // +0x61c
};
#pragma pack(pop)

Display_004be400* FUN_004b6220();
Entry_004be400* __stdcall FUN_004bb4e0(Table_004be400* table, char* name);
int __stdcall FUN_004bc4b0(const char* path, struct _finddata_t* fd, int state, char recursive);
int __stdcall FUN_004bc640(int handle, struct _finddata_t* fd);
void FUN_004d85a0(void* p);

// FUNCTION: 0x4be400
void __stdcall FUN_004be400(char* path, int state, int recursive)
{
    Display_004be400* d = FUN_004b6220();
    char buf[0x100];
    struct _finddata_t fd;

    strcpy(buf, path);
    strcat(buf, "*");
    int h = FUN_004bc4b0(buf, &fd, state, recursive);
    if (h == -1)
        return;
    do {
        if (fd.attrib & 0x10) {
            if (strcmp(fd.name, ".") != 0 && strcmp(fd.name, "..") != 0) {
                strcpy(buf, path);
                strcat(buf, fd.name);
                strcat(buf, "\\");
                FUN_004be400(buf, ((Find_004be400*)h)->state, 0);
            }
        } else {
            int i = ((Find_004be400*)h)->state;
            if (i < 0)
                i = 0;
            else
                i++;
            for (; i < d->count; i++) {
                strcpy(buf, path);
                strcat(buf, fd.name);
                Entry_004be400* e = FUN_004bb4e0(d->files[i]->header->table, buf);
                if (e && !(e->flags & 1))
                    e->flags |= 2;
            }
        }
    } while (FUN_004bc640(h, &fd) != -1);
    Find_004be400* f = (Find_004be400*)h;
    if (f) {
        if (f->state < 0)
            _findclose(f->handle);
        FUN_004d85a0(f);
    }
}
