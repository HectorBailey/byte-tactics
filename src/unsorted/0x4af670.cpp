// Decompiled by space-bunny-free. Names are provisional.
//
// Click handler of the file requester (FILEREQ.GUI, opened by 0x4afa30).
// On close (field_60 == -1) it restores the saved drive and directory and
// frees the request data. Otherwise it acts on the entry the user clicked:
// LOAD/SWIN enter a directory, CANC accepts, NAME takes the highlighted file,
// PATH walks one level up and the *DRV entries pick a drive letter.
//
// 99.1% (check.py), 945 bytes, exactly the original's size. Only three
// instructions differ, all in the PATH branch, and only in the SIB byte: the
// original wants the frame-pointer register in the BASE slot, ours puts it in
// the INDEX slot.
//   0x4af749  cmp byte ptr [ebp+ecx*1+0x23], 0x3a   (ours [ecx+ebp*1+0x23])
//   0x4af751  mov byte ptr [ebp+ecx*1+0x25], 0      (ours [ecx+ebp*1+0x25])
//   0x4af761  mov byte ptr [ebp+ecx*1+0x24], 0      (ours [ecx+ebp*1+0x24])
// The address is `req->cwd[n-1]`, `req->cwd[n+1]` and `req->cwd[n]` with `req`
// in ebp and `n` in ecx, so both spellings compute the same address. The loop
// test two instructions earlier (0x4af739, `req->cwd[n]`) matches: there the
// compiler reuses the `lea edx,[ebp+0x24]` value, so edx is the base and ecx
// the index. So the shape is right and only combine()'s choice of base and
// index differs. Untried: about 60 source spellings of the three statements
// (array subscript, byte-offset casts, `&req->cwd[n] +- 1`, a `char* p =
// req->cwd + n` with p[-1]/p[1]/p[0], an if/else and a ternary for the tail,
// unsigned indices, every declaration order and scope for `n`, `i`, `name`,
// `entries`, `drive` and `result`) and all 768 header sets headers.py tried
// (the 128 common ones, and those crossed with <string>, <vector>, <map>,
// <list> and <iostream>) all produce the same 3 bytes.
//
// What did fix the rest (was 90%): the LOAD/SWIN block needs no `tail` local
// at all. `strcpy(req->cwd + strlen(req->cwd) - 1, tail)` made the compiler
// hoist the destination above the source scan and spill it; writing the append
// as the two `strcat` calls of the `strlen(cwd) == 3` test instead
// (`strcat(req->cwd, req->selected + 1)` / `strcat(req->cwd, req->selected)`)
// reproduces the original's `dec edi` destination, its ebx length park and its
// edi tail pointer exactly, with no spill and no extra move.
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
            if ((int)strlen(req->cwd) == 3) {
                strcat(req->cwd, req->selected + 1);
            } else {
                strcat(req->cwd, req->selected);
            }
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
