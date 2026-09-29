// Decompiled by space-bunny-free. Names are provisional.
// 92.7 percent: 638 of the original's 642 bytes. The four missing bytes are one
// dead instruction in the epilogue; everything else that differs is a register
// or an instruction ordering, not logic.
//
// WHAT THE ORIGINAL DOES, all of it read off the bytes:
//  - six dword arguments, all six read: (char* path, char* list, char* sizes,
//    int mode, int flag, int what);
//  - the frame is 0x22c: count at +0x10, ONE search handle at +0x14 (both
//    searches store that same slot, so the source has a single function scope
//    handle), the times base at +0x18, a saved copy of list at +0x1c, the
//    running times pointer at +0x20, an io.h _finddata_t at +0x24 and a
//    char[256] buffer at +0x13c that _itoa writes the file size into;
//  - mode 1 walks the subdirectories of the search 0x4bc4b0 opened on "*."
//    and appends "\\name" plus a "<DIR>" marker per entry; anything else, and
//    mode 1 as well, lists the plain files of path, optionally cutting the
//    extension (FUN_004bb0f0 when flag is 1) and recording the write time and
//    the size (FUN_004bbc40 through _itoa) per entry;
//  - the mode test and the handle test are ONE condition: both false tests jump
//    to the same one-instruction block at +0x438, the load of the sizes
//    pointer, and the completed directory walk jumps over it to +0x43f. So the
//    file walk runs in every case, and with mode 1 the sizes list is continued
//    from wherever the directory walk left it. That is a bug in Cavedog's code
//    (or at least an accident worth writing down): the directory walk is not an
//    alternative to the file walk, it runs in front of it;
//  - the collector 0x4aefa0 is called at the end with (list, 0, times, count),
//    except when what is 2, when the times pointer is 0 as well.
//
// WHAT IS STILL WRONG HERE, three items, all allocation:
//  1. the first search loads the sizes pointer before the handle test, the
//     original after it (the second search's load is right in both);
//  2. the count read-modify-write at the end of each loop body goes through edi
//     here and through eax in the original, and the original's store sits
//     before the two argument pushes where this file's sits between them. The
//     count is memory resident in both, so this is a scratch choice;
//  3. the original reloads the now dead handle slot into eax right after its
//     last call (mov eax, [esp+0x14] at 0x4af58a) and that load is the whole
//     four byte shortfall. Nothing I tried reproduces it, see below.
//
// THE ONE SOURCE DETAIL THAT WAS WORTH THE LAST 1.9 POINTS: the search handle
// is ONE function scope variable shared by both searches, not a pair of block
// scoped ones. Both searches then store the same slot, and the saved copy of
// list and the times base land in the slots the original uses. This is the
// guide's item 2: the two block scoped handles were two live graph nodes, and
// merging them demoted a variable one step.
//
// Tried with no effect this round: the declaration order of the locals (times
// before the saved copy, the count first, the handle at the top of the
// function), the handle as unsigned, the count as unsigned int, grouping times,
// the saved copy and the running pointer in one struct (78.5 percent, the
// struct is scalar replaced), a local pointer to the _finddata_t (identical),
// a block scoped or function scope copy of the sizes parameter for one or for
// both searches (77.6 and 86.7 percent, the copies stop the second load from
// landing where the original has it), and a dead read of the handle after the
// free.
#include <stdlib.h>
#include <string.h>
#include <io.h>

void* FUN_004d83b0(char* name, unsigned int size);
void FUN_004d85a0(void* p);
int __stdcall FUN_004aefa0(char* names, char* sizes, void* times, int count);
int __stdcall FUN_004bc4b0(const char* path, struct _finddata_t* fd, int state, char recursive);
int __stdcall FUN_004bc640(int handle, struct _finddata_t* fd);
void __stdcall FUN_004bc8d0(int handle);
char* __stdcall FUN_004bb0f0(char* name);
long __stdcall FUN_004bbc40(char* name);

// FUNCTION: 0x4af320
void __stdcall FUN_004af320(char* path, char* list, char* sizes, int mode, int flag, int what)
{
    char* first = list;
    int num = 0;
    char* times = (char*)FUN_004d83b0("FILETIMES", 0x2ee0);
    long* tp = (long*)times;
    struct _finddata_t fd;
    char text[256];
    int find;

    if (mode == 1) {
        find = FUN_004bc4b0("*.", &fd, -1, 1);
        if (find != -1)
            do {
                if (fd.name[0] != '.' && fd.attrib == 0x10) {
                    strncpy(list, "\\", 1);
                    list++;
                    strcpy(list, fd.name);
                    list += strlen(fd.name);
                    *list++ = 0;
                    if (sizes) {
                        strcpy(sizes, "<DIR>");
                        sizes += 6;
                    }
                }
                num++;
            } while (FUN_004bc640(find, &fd) != -1);
        FUN_004bc8d0(find);
    }
    {
        find = FUN_004bc4b0(path, &fd, -1, 1);
        if (find != -1)
            do {
                if (fd.name[0] != '.' && fd.attrib != 0x10) {
                    strcpy(list, fd.name);
                    if (flag == 1) {
                        FUN_004bb0f0(list);
                    }
                    list += strlen(list) + 1;
                    *tp++ = fd.time_write;
                    if (sizes) {
                        _itoa(FUN_004bbc40(fd.name), text, 10);
                        strcpy(sizes, text);
                        sizes += strlen(text) + 1;
                    }
                }
                num++;
            } while (FUN_004bc640(find, &fd) != -1);
        FUN_004bc8d0(find);
    }
    if (what) {
        if (what == 2) {
            FUN_004aefa0(first, 0, 0, num);
        } else {
            FUN_004aefa0(first, 0, times, num);
        }
    }
    FUN_004d85a0(times);
    *list = 0;
}
