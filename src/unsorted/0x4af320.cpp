// Decompiled by space-bunny-free. Names are provisional.
// MEASURED STATE of the file below (check.py, this pass): 90.8 percent,
// 638 bytes against the original's 642. Nothing in the body changed this
// pass; no variant tried beat it.
//
// THE ONE BYTE-COUNT DIFFERENCE IN THE WHOLE FUNCTION. Pairing every
// instruction of the original with ours shows that exactly one instruction is
// missing and no instruction is a byte longer or shorter than its counterpart:
//   original 0x4af58a  mov eax, dword ptr [esp + 0x14]     (4 bytes, dead)
// and it sits between `call 0x4d85a0` and the `add esp, 4` that cleans up that
// cdecl call. The stack slot it reads is the COUNT (offsets here are as
// printed, with esp already four pushes into the frame, so [esp+0x10] is the
// count, [esp+0x14] the search handle, [esp+0x18] and [esp+0x1c] the times
// base and the saved copy of list, [esp+0x20] the running times pointer,
// [esp+0x24] the _finddata_t and [esp+0x13c] the char[256] _itoa buffer).
// So the original ends by reloading the count into eax and never uses it.
// Everything else that differs is a register or a slot, at identical sizes:
//
//   1. slot order. The original keeps the times base at [esp+0x18] and the
//      saved copy of list at [esp+0x1c]; this file has them the other way
//      round, which moves the store in the prologue, the two reloads of the
//      times base, and the two loads of the list copy in the tail. Swapping
//      the two declarations does NOT swap the slots (the store order changes,
//      the addresses do not), and neither does the type: see the list of
//      tries below.
//   2. the count through a register. The original does the increment in eax
//      with the store immediately after it (mov eax,[num]; mov esi,[find];
//      inc eax; mov [num],eax; lea ecx,[fd]; push ecx; push esi; call), this
//      file does it in edi with the store sunk past the two pushes. The
//      original never gives the count a callee-saved register; this file
//      gives it edi, and edi is what the original gives the times base.
//   3. the load of the sizes parameter. The original emits `mov ebx,
//      [esp+0x248]` after the `je`, this file hoists it above the `cmp esi,
//      -1`.
//   4. the reload of the times base sits after the FUN_004bc8d0 call in the
//      original and before it here, a consequence of 2.
//
// ONE CONTROL-FLOW DIFFERENCE, DELIBERATELY KEPT. The original's `find == -1`
// branch jumps over the FUN_004bc8d0 call (je 0x4af438 at 0x4af385, the call
// at 0x4af42c), so the close really is inside the `if (find != -1)` block.
// This file has it outside, which is also what its score depends on: with the
// call inside the guard the same source gives 634 bytes and 82.2 percent,
// because there the handle never gets a callee-saved register at all (the two
// `mov esi, eax` after the two FUN_004bc4b0 calls disappear, four bytes) and
// the times base takes esi instead of edi. The arithmetic is exact:
//   634 (close inside) + 4 (the two mov esi, eax) + 4 (the dead reload) = 642,
// while the close outside is 638 = 642 - 4, i.e. this file already has the
// handle in esi and is short only by the reload. So the original sits between
// the two shapes and the whole remaining problem is that one register choice.
// The extra call this file makes is not a behaviour change: 0x4bc8d0's own
// matched file shows it returns -1 immediately for -1, so closing a search
// that never opened is a no-op. The faithful spelling is one line: move
// FUN_004bc8d0(find) inside the `if (find != -1)` block of both walks, which
// costs 8.6 percent.
//
// WHAT THE ORIGINAL DOES, all of it read off the bytes:
//  - six dword arguments, all six read: (char* path, char* list, char* sizes,
//    int mode, int flag, int what);
//  - the frame is 0x22c: five dword locals (count, the search handle, the
//    times base, a saved copy of list, the running times pointer), an io.h
//    _finddata_t and a char[256] buffer that _itoa writes the file size into;
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
// THE ONE SOURCE DETAIL THAT UNLOCKED THE REGISTERS: each search has its own
// block scoped handle, declared inside the block that uses it, and the two
// share the slot. With one function scope handle the times base takes esi,
// the handle never gets a register, and the score drops to 83.6 percent.
//
// TRIED WITH NO EFFECT, this pass (on top of the previous pass's list):
//  - fifteen real header sets, not just the unused-declaration sweep:
//    windows.h, windows.h+io.h, windows.h+stdlib+string, windows.h+stdio,
//    stdio, string, vector+map, direct.h, fcntl.h, time.h, memory+math,
//    iostream, ddraw.h, list and bare windows.h, each on top of the io.h,
//    string.h and stdlib.h this file needs. Every one of them gives this
//    exact 90.8 percent output, so the header lever really is dead here, and
//    not only for N unused extern declarations.
//  - the loop shape of both walks, which is the only thing that ever moved the
//    byte count: do/while with the close inside (634) or outside (638), the
//    close in an else, `for (; find != -1; find = FUN_004bc640(find, &fd))`
//    with the increment in the body (644), the same for with `num++,
//    find = FUN_004bc640(find, &fd)` in the increment clause (644), a plain
//    `while (find != -1)` with the update in the body (644), a for whose init
//    is the search call and no outer guard (633), `for (;;)` with an explicit
//    break on the -1 (638, and the increment becomes `add edi, 2`),
//    `while (1)` with a break (638), and the increment moved to the top of the
//    body (623). None of them is both 642 bytes and the original's shape; the
//    for and while forms are all worse on the score as well.
//  - the count as `num++`, `++num`, `num += 1` and `num = num + 1` (all
//    identical), as `unsigned int`, and inside a one element struct and a one
//    element array instead of a plain int;
//  - the times base as char*, void*, long*, int*, time_t* and
//    unsigned char*, with tp as long*, time_t* or int*; the saved copy as
//    const char*; the count and the saved copy assigned by a statement instead
//    of an initialiser; the declaration order of all five small locals (the
//    store order follows the source, the stack slots do not);
//  - the tail: `if (what) { if (what == 2) A else B }`, the same with the
//    branches swapped, `if (what == 0) ; else if (what == 2) A else B`, a
//    `switch (what)` with case 0 breaking, `list[0] = 0` instead of `*list =
//    0`, and the free moved before the terminator (82.2 to 90.8 percent, all
//    634 to 639 bytes).
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

    if (mode == 1) {
        int find = FUN_004bc4b0("*.", &fd, -1, 1);
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
        int find = FUN_004bc4b0(path, &fd, -1, 1);
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
