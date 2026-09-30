// Decompiled by deepseek-v4.1-flash, rechecked by deepseek-v4.1. Names are provisional.
// Partial, 78.6% (1154 of 1156 bytes). Everything below the entry block has the
// right fields, frame (0x214), memset sizes (0x13e + 0xcc) and call order; the
// whole gap is ONE register swap in the allocator:
//   original: EBX = the live zero (cmp eax,ebx / mov [..],ebx / mov [..],bl),
//             ESI = cached _imp__wsprintfA reloaded at the loop head;
//   ours:     ESI = the live zero (xor esi,esi, re-materialised at the loop
//             head because the inlined strcpy kills ESI), EBX = the cached
//             wsprintfA import, so ours emits `call ebx` where the original
//             reloads `call dword ptr [0x4fc2e8]` after the strcpy clobbers ESI.
// Consequences of the swap, all visible in the checker diff: `mov [..],0` /
// `test eax,eax` in ours where the original uses bl/ebx, `mov esi,0x14` and
// `mov [..],si` where the original uses edi/di, and `mov ebx,[0x511de8]` at
// entry where the original has `mov esi,[0x511de8]`.
// The block that decides it is the entry: ours gives the g_game temporary EBX
// and the zero ESI, the original the reverse, so an attempt should target the
// first-use order at entry, not the loop body. See build/scratch/0x479c50/d1.txt
// for the full checker diff.
// Further attempts (all scored with check.py --sym, all 78.6% or worse):
//   headers.py: no header set matches (best is the current <windows.h>).
//   N unused extern declarations 0..200: flat at 78.6%, so it is source shape,
//   not compiler state.
//   loop counter declared early / unsigned, named numPlayers local (51.5%),
//   flags-before-loop reordered (78.3%), |= vs = x|1, while loop, step/y after
//   the memsets, implicit null tests, menu local pointer (64.7%): none flipped
//   the ESI/EBX swap.
// Root cause narrowed with /Fa listings: the whole diff follows from which
// register the g_game temporary gets. When g_game lands in ESI the allocator
// then gives the live zero EBX and the wsprintf import reuses ESI after g_game
// dies at the loop guard, exactly as the original. When g_game lands in EBX the
// zero takes ESI and the import reuses EBX (the current state). So the task is
// to make g_game outrank the constant zero.
// Things that DO flip g_game to ESI but break the match: an explicit
// `if (g_game->numPlayers <= 0) return;` before the loop (adds a second cmp,
// 73.5%), a local `int count = g_game->numPlayers;` used as the bound (adds a
// 4-byte frame slot, 0x218), a live `menu` local (64.7%), a `Game* p = g_game;`
// local (59.5%). None of these is the original: the frame must stay 0x214 and
// there must be exactly one guard cmp. The missing lever is a g_game use that
// raises its priority without emitting an extra instruction, and it was not
// found in this session.
// New probe (second session): `Rec1 rec1 = {0}; Rec2 rec2 = {0};` instead of
// the two memset calls DOES move the g_game temporary into ESI (the 0x479c59
// shape), but it also folds the numPlayers load into `idiv dword ptr
// [esi+0x38d81]`, drops the early `mov ecx,[esi+0x38d81]`, pushes EBP before
// ESI and leaves the wsprintfA import in EBX, so the entry block is worse.
// Rejected; best remains the variant below.
#include <windows.h>
#include <string.h>

struct GafEntry_004b8d40;
struct Gaf_004b8d40;
GafEntry_004b8d40* __stdcall FUN_004b8d40(Gaf_004b8d40* gaf, const char* name);
int __stdcall FUN_004b7f30(unsigned short* param_1, int param_2);
char* __stdcall FUN_004c5740(char* key);
int __stdcall FUN_004ab2b0(void* obj, void* record);
int __stdcall FUN_004ab310(void* obj, void* record);

#pragma pack(push, 1)
struct Header_00479c50 {
    unsigned char type;                // +0x00
    unsigned char group;               // +0x01
    char name[0x11];                   // +0x02
    short x;                           // +0x13
    short y;                           // +0x15
    short w;                           // +0x17
    short h;                           // +0x19
    int attr;                          // +0x1b
    int color;                         // +0x1f
    int color2;                        // +0x23
    char unknown_27[2];                // +0x27
    unsigned char flag;                // +0x29
};

struct Rec1_00479c50 {                 // 0x13e bytes
    Header_00479c50 h;                 // +0x00
    char unknown_2a[0x2f - 0x2a];
    void* entry;                       // +0x2f
    char text[0x103];                  // +0x33
    unsigned char f136;                // +0x136
    char pad_137;
    short f138;                        // +0x138
    char pad_13a;
    unsigned char frame;               // +0x13b
    char pad_13c[2];
};

struct Rec2_00479c50 {                 // 0xcc bytes
    Header_00479c50 h;                 // +0x00
    char unknown_2a[0x33 - 0x2a];
    char text[0x83];                   // +0x33
    short count;                       // +0xb6
    char unknown_b8[0xc8 - 0xb8];
    int flags;                         // +0xc8
};

struct Entries_00479c50 {
    char unknown_0[0xc0];
    Gaf_004b8d40* gaf;                 // +0xc0
};

struct Holder_00479c50 {
    char unknown_0[4];
    Entries_00479c50* entries;         // +0x04
};

struct Menu_00479c50 {
    char unknown_0[0x18];
    Holder_00479c50* holder;           // +0x18
};

struct Game_00479c50 {
    char unknown_0[0x519];
    Menu_00479c50 menu;                // +0x519
    char unknown_535[0x38d81 - 0x535];
    int numPlayers;                    // +0x38d81
};
#pragma pack(pop)

extern Game_00479c50* g_game;

static void __stdcall SetEntry_00479c50(Rec1_00479c50* obj, char* name)
{
    Holder_00479c50* holder = g_game->menu.holder;
    obj->entry = 0;
    Gaf_004b8d40* gaf = holder->entries->gaf;
    if (gaf != 0) {
        GafEntry_004b8d40* e = FUN_004b8d40(gaf, name);
        if (e != 0) {
            obj->entry = e;
            short* f = (short*)FUN_004b7f30((unsigned short*)e, obj->frame);
            if (f != 0) {
                obj->h.w = f[0];
                obj->h.h = f[1];
            }
        }
    }
}

// FUNCTION: 0x479c50
void FUN_00479c50(void)
{
    int step = 200 / g_game->numPlayers;
    Rec1_00479c50 rec1;
    Rec2_00479c50 rec2;
    int y = (0xb4 - (g_game->numPlayers - 1) * step) / 2 + 0x4f;
    memset(&rec1, 0, sizeof(rec1));
    memset(&rec2, 0, sizeof(rec2));
    rec1.h.flag = 1;
    rec2.h.flag = 1;
    rec2.flags |= 1;
    for (int i = 0; i < g_game->numPlayers; i++) {
        rec1.h.y = y;
        rec2.h.y = y;
        wsprintfA(rec1.h.name, "Player%d", i);
        rec1.h.x = 0x2d;
        rec1.h.w = 0x70;
        rec1.h.h = 0x14;
        rec1.h.attr = 2;
        rec1.text[0] = 0;
        SetEntry_00479c50(&rec1, "skirmname");
        FUN_004ab2b0(&g_game->menu, &rec1);

        wsprintfA(rec1.h.name, "Side%d", i);
        rec1.h.x = 0xa3;
        rec1.h.w = 0x2d;
        SetEntry_00479c50(&rec1, "SIDEx");
        rec1.frame = 0;
        rec1.f136 = 2;
        FUN_004ab2b0(&g_game->menu, &rec1);

        wsprintfA(rec2.h.name, "Color%d", i);
        rec2.h.x = 0xd6;
        rec2.h.w = 0x14;
        rec2.h.h = 0x14;
        rec2.text[0] = 0;
        FUN_004ab310(&g_game->menu, &rec2);

        wsprintfA(rec2.h.name, "Allies%d", i);
        rec2.h.x = 0xf1;
        rec2.h.w = 0x28;
        rec2.h.h = 0x14;
        strcpy(rec2.text, FUN_004c5740("Click to select an allegiance symbol."));
        FUN_004ab310(&g_game->menu, &rec2);

        wsprintfA(rec1.h.name, "Metal%d", i);
        rec1.h.attr |= 0x10000;
        rec1.h.x = 0x11e;
        rec1.h.w = 0x2d;
        rec1.h.h = 0x14;
        rec1.f136 = 0;
        rec1.f138 = 0;
        SetEntry_00479c50(&rec1, "skirmmet");
        strcpy(rec1.text, FUN_004c5740("Left click to increase metal. Right click to decrease metal."));
        FUN_004ab2b0(&g_game->menu, &rec1);

        wsprintfA(rec1.h.name, "Energy%d", i);
        rec1.h.x = 0x151;
        rec1.h.w = 0x2d;
        SetEntry_00479c50(&rec1, "skirmmet");
        strcpy(rec1.text, FUN_004c5740("Left click to increase energy. Right click to decrease energy."));
        FUN_004ab2b0(&g_game->menu, &rec1);
        y += step;
    }
}
