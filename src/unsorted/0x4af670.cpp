// Decompiled by space-bunny-free. Names are provisional.
// Click handler of the file requester (FILEREQ.GUI, opened by 0x4afa30).
// On close (field_60 == -1) it restores the saved drive and directory and
// frees the request data. Otherwise it acts on the entry the user clicked:
// LOAD/SWIN enter a directory, CANC accepts, NAME takes the highlighted file,
// PATH walks one level up and the *DRV entries pick a drive letter.
//
// 99.1% (check.py), and the total size is now exact: 945 bytes, same as the
// original. Only three instructions in the PATH branch still differ, and they
// are all the same defect: MSVC 5 puts the loop counter in the SIB *base* slot
// and `req` in the *index* slot, where the original does the opposite.
//   original  cmp byte [ebp + ecx + 0x23], 0x3a   SIB 0x0d  (base ebp, index ecx)
//   ours      cmp byte [ecx + ebp + 0x23], 0x3a   SIB 0x29  (base ecx, index ebp)
// Same address, same length, opposite operand slots; it happens for all three
// accesses (`cwd[n-1]`, `cwd[n]`, `cwd[n+1]`), and the plain `cwd[n]` in the
// loop head already agrees (`cmp byte [ecx + edx], 0x5c`, base edx). So the
// difference is only in which operand MSVC's address generator promotes to
// base once the constant is folded into the displacement. Six source shapes
// for this branch (the subscript form, `*(req->cwd + n +- 1)`, a block-local
// `char* cwd`, a `char* p = req->cwd + n` walked down, a named `int m = n - 1`,
// and one where the count is read through a local) all keep the swapped slots
// or change the whole block shape, so this looks like an LTG tie-break that
// needs a different expression tree, not a different type.
//
// Decompiled by space-bunny-free. Names are provisional.
#include <string.h>

#pragma pack(push, 1)
struct Entry_004af670 {              // 0x15b bytes
    char unknown_0[2];
    char name[0x10];                 // +0x02
    char unknown_12[0xb6 - 0x12];
    char value[0xba - 0xb6];         // +0xb6
    short field_ba;                  // +0xba
    char unknown_bc[0x15b - 0xbc];
};

struct Req_004af670 {
    char unknown_0[0x24];
    char cwd[0x100];                 // +0x24
    char save_drive[0x10];           // +0x124
    char save_cwd[0x100];            // +0x134
    char* names;                     // +0x234
    char* sizes;                     // +0x238
    char* selected;                  // +0x23c
    int field_240;                   // +0x240
    void (__stdcall* callback)(void*);   // +0x244
};

struct Layer_004af670 {
    char unknown_0[4];
    Entry_004af670* entries;         // +0x04
    char unknown_8[4];
    Req_004af670* req;               // +0x0c
};

struct Gadget_004af670 {
    char unknown_0[0x18];
    Layer_004af670* layer;           // +0x18
    char unknown_1c[0x60 - 0x1c];
    int field_60;                    // +0x60
};
#pragma pack(pop)

int __stdcall FUN_004a0300(Entry_004af670* entries, int i, char* name);
int __stdcall FUN_0049fdf0(Entry_004af670* entries, char* name, int type);
Entry_004af670* __stdcall FUN_0049ff90(Entry_004af670* entries, char* name);
void __stdcall FUN_0049fa90(Gadget_004af670* gadget);
void __stdcall FUN_004ab0a0(Gadget_004af670* gadget);
void __stdcall FUN_004af5b0(Req_004af670* req);
char* __stdcall FUN_004b6af0(char* text, int n);
int __stdcall FUN_004bc300(char* drive);
void __stdcall FUN_004bc360(const char* path);
void __cdecl FUN_004d85a0(void* data);

// FUNCTION: 0x4af670
void __stdcall FUN_004af670(Gadget_004af670* gadget)
{
    Req_004af670* req = gadget->layer->req;
    if (gadget->field_60 == -1) {
        FUN_004bc300(req->save_drive);
        FUN_004bc360(req->save_cwd);
        FUN_004d85a0(req);
        return;
    }

    Entry_004af670* entries = gadget->layer->entries;
    char drive[2];
    drive[1] = 0;
    int result = 0;

    if (FUN_004a0300(entries, gadget->field_60, "LOAD")
        || FUN_004a0300(entries, gadget->field_60, "SWIN")) {
        char* name = FUN_004b6af0(req->names,
                                  FUN_0049ff90(entries, "SWIN")->field_ba);
        if (name[0] == '\\') {
            strcpy(req->selected, name);
            int i;
            for (i = 0; i < 10; i++) {
                if (req->selected[i] == ' ') {
                    req->selected[i] = 0;
                    break;
                }
            }
            int n = (int)strlen(req->cwd);
            char* tail = req->selected;
            if (n == 3) {
                tail++;
            }
            strcat(req->cwd, tail);
            FUN_004bc360(req->cwd);
        } else {
            result = 1;
            strcpy(req->selected, req->cwd);
            strcat(req->selected, "\\");
            strcat(req->selected, name);
        }
    } else if (FUN_004a0300(entries, gadget->field_60, "CANC")) {
        result = 1;
    } else if (FUN_004a0300(entries, gadget->field_60, "PATH")) {
        int n = (int)strlen(req->cwd);
        if (n > 0) {
            while (n > 0) {
                if (req->cwd[n] == '\\') {
                    if (req->cwd[n - 1] == ':') {
                        req->cwd[n + 1] = 0;
                        FUN_004bc360(req->cwd);
                    } else {
                        req->cwd[n] = 0;
                        FUN_004bc360(req->cwd);
                    }
                    break;
                }
                n--;
            }
        }
    } else if (FUN_004a0300(entries, gadget->field_60, "NAME")) {
        gadget->field_60 = FUN_0049fdf0(entries, "LOAD", 14);
        int n = FUN_0049fdf0(entries, "NAME", 3);
        result = 1;
        strcpy(req->selected, entries[n].value);
    } else if (FUN_004a0300(entries, gadget->field_60, "ADRV")) {
        drive[0] = 'A';
        FUN_004bc300(drive);
    } else if (FUN_004a0300(entries, gadget->field_60, "BDRV")) {
        drive[0] = 'B';
        FUN_004bc300(drive);
    } else if (FUN_004a0300(entries, gadget->field_60, "CDRV")) {
        drive[0] = 'C';
        FUN_004bc300(drive);
    } else if (FUN_004a0300(entries, gadget->field_60, "DDRV")) {
        drive[0] = 'D';
        FUN_004bc300(drive);
    } else if (FUN_004a0300(entries, gadget->field_60, "VDRV")) {
        drive[0] = 'R';
        FUN_004bc300(drive);
    }

    if (result == 1) {
        if (req->callback) {
            req->callback(req);
        }
    } else {
        FUN_004af5b0(req);
        FUN_0049fa90(gadget);
        FUN_004ab0a0(gadget);
    }
}
