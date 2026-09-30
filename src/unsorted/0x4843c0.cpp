// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, finished by GPT-6. Names are provisional.
// Gave up at 86.7% (1302 bytes against 1303). Everything from the memset's
// epilogue to the final pop matches byte for byte; the single difference is
// the register of the reloaded game pointer at 0x48441b, where the original
// has `mov edi, [g_game]` and this file has `mov eax, [g_game]`. That one
// byte is why every later branch target is off by one. See the note at the
// end of the file for everything that was tried.
#include <windows.h>

#pragma pack(push, 1)

struct Cell {
    unsigned char lo;                  // +0x0
    unsigned char hi;                  // +0x1
};

struct Grid {
    Cell* cells;                       // +0x0
    unsigned int width;                // +0x4
    unsigned int height;               // +0x8
    int count;                         // +0xc
};

struct PlayerGrid {
    unsigned char* cells;              // +0x0
    unsigned int width;                // +0x4
    unsigned int height;               // +0x8
    char pad[0x14b - 0xc];
};

struct MapInfo {
    char pad0[0x24];
    Grid* grid;                        // +0x24
    char pad1[0x38 - 0x28];
    int width;                         // +0x38
    int height;                        // +0x3c
    char pad2[0x78 - 0x40];
    unsigned short* visibilityMask;    // +0x78
};

union FlagWord {
    unsigned short raw;
    struct {
        unsigned short bit0 : 1;
        unsigned short bit1 : 1;       // mask 2
        unsigned short rest : 14;
    } bits;
};

struct Game {
    char pad0[0x1bdf];
    PlayerGrid players[10];            // +0x1bdf, stride 0x14b
    char pad1[0x2a43 - (0x1bdf + 10 * 0x14b)];
    unsigned char playerIndex;         // +0x2a43
    char pad2[0x141fb - 0x2a44];
    MapInfo info;                      // +0x141fb
    char pad3[0x14281 - (0x141fb + 0x7c)];
    FlagWord flags;                    // +0x14281
    char pad4[0x1431f - 0x14283];
    int scrollX;                       // +0x1431f
    int scrollY;                       // +0x14323
};

#pragma pack(pop)

extern Game* g_game;

// FUNCTION: 0x4843c0
void FUN_004843c0(void)
{
    MapInfo* info = &g_game->info;
    Grid* grid = info->grid;
    PlayerGrid* pg = &g_game->players[g_game->playerIndex];
    unsigned int bit = 1 << g_game->playerIndex;

    memset(grid->cells, 0, grid->count * 2);

    int x0 = g_game->scrollX;
    int y0 = g_game->scrollY;
    int ry = y0 % 32;
    int rx = x0 % 32;
    if (rx < 16) x0 = x0 / 32 - 1; else x0 = x0 / 32;
    if (ry < 16) y0 = y0 / 32 - 1; else y0 = y0 / 32;

    int xEnd = x0 + grid->width;
    int yEnd = y0 + grid->height;

    for (int y = y0; y < yEnd; y++) {
        for (int x = x0; x < xEnd; x++) {
            if (x < pg->width && y < pg->height) {
                if (pg->cells[y * pg->width + x] == 0 && (g_game->flags.raw & 2)) {
                    if (x - x0 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0].hi |= 1;
                    if (x - x0 - 1 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0 - 1].hi |= 2;
                    if (x - x0 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0].hi |= 4;
                    if (x - x0 - 1 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0 - 1].hi |= 8;
                }
                if ((unsigned short)(bit & info->visibilityMask[info->width * y / 2 + x]) == 0) {
                    if (x - x0 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0].lo |= 1;
                    if (x - x0 - 1 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0 - 1].lo |= 2;
                    if (x - x0 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0].lo |= 4;
                    if (x - x0 - 1 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0 - 1].lo |= 8;
                }
            }
        }
    }

    if (y0 < 0) {
        for (unsigned int i = 0; i < grid->width; i++) {
            if (g_game->flags.bits.bit1) {
                if (grid->cells[i].hi & 4) grid->cells[i].hi |= 1;
                if (grid->cells[i].hi & 8) grid->cells[i].hi |= 2;
            }
            if (grid->cells[i].lo & 4) grid->cells[i].lo |= 1;
            if (grid->cells[i].lo & 8) grid->cells[i].lo |= 2;
        }
    }

    if (yEnd > info->height / 2) {
        for (unsigned int i = 0; i < grid->width; i++) {
            if (g_game->flags.bits.bit1) {
                if (grid->cells[(grid->height - 2) * grid->width + i].hi & 1)
                    grid->cells[(grid->height - 2) * grid->width + i].hi |= 4;
                if (grid->cells[(grid->height - 2) * grid->width + i].hi & 2)
                    grid->cells[(grid->height - 2) * grid->width + i].hi |= 8;
            }
            if (grid->cells[(grid->height - 2) * grid->width + i].lo & 1)
                grid->cells[(grid->height - 2) * grid->width + i].lo |= 4;
            if (grid->cells[(grid->height - 2) * grid->width + i].lo & 2)
                grid->cells[(grid->height - 2) * grid->width + i].lo |= 8;
        }
    }

    if (x0 < 0) {
        for (unsigned int i = 0; i < grid->height; i++) {
            if (g_game->flags.bits.bit1) {
                if (grid->cells[i * grid->width].hi & 8)
                    grid->cells[i * grid->width].hi |= 4;
                if (grid->cells[i * grid->width].hi & 2)
                    grid->cells[i * grid->width].hi |= 1;
            }
            if (grid->cells[i * grid->width].lo & 8)
                grid->cells[i * grid->width].lo |= 4;
            if (grid->cells[i * grid->width].lo & 2)
                grid->cells[i * grid->width].lo |= 1;
        }
    }

    if (xEnd > info->width / 2) {
        for (unsigned int i = 1; i - 1 < grid->height; i++) {
            if (g_game->flags.bits.bit1) {
                if (grid->cells[i * grid->width - 2].hi & 4)
                    grid->cells[i * grid->width - 2].hi |= 8;
                if (grid->cells[i * grid->width - 2].hi & 1)
                    grid->cells[i * grid->width - 2].hi |= 2;
            }
            if (grid->cells[i * grid->width - 2].lo & 4)
                grid->cells[i * grid->width - 2].lo |= 8;
            if (grid->cells[i * grid->width - 2].lo & 1)
                grid->cells[i * grid->width - 2].lo |= 2;
        }
    }
}

// Notes for whoever finishes this (space-bunny-free, second pass).
//
// The original at 0x48441b is
//     mov edi, [g_game]
//     mov ebx, [edi+0x14323]        ; scrollY into y0's home
//     mov edi, [edi+0x1431f]        ; scrollX into x0's home
// so the two-use game-pointer temporary shares x0's register, and this
// version's `mov eax, [g_game]` is the same three instructions in every other
// respect. Nothing in the rest of the function differs once that byte is
// accounted for: the 8 local slots, the two clamps, the visibility test, all
// four border loops and the flags bit test are all byte exact.
//
// Tried, none of which moved the base out of EAX (all free scratch scoring,
// 86.7% every time): swapping the two reads; a `Game*` local for the two
// reads alone, and one also used by the flags test; the same local declared
// and live from the top of the function (that does put the pointer in EDX and
// gives the right ebx/edi destinations, but drops the reload, so it is 3
// bytes short); reading the pair as `int sc[2]` or as a two-int struct; an
// inlined helper with out-parameters; an inline accessor per field; putting
// the `% 32` inline in the condition; unsigned locals; `(char)`/unsigned
// casts; the clamps via an inline helper; extra live references before or
// after the reload; dead filler statements between the two reads; reordering
// the prologue; a named `cells` pointer for the memset; and the memset by
// hand. headers.py found nothing either (all 128 sets score 86.7%).
//
// So the base of a double indirection through a global pointer is put in EAX
// here whatever the source says, and the original's EDI has to come from a
// construct not guessed yet. A local pointer that really is live is kept in a
// scratch register (EDX in the experiment above), so it is not simply a
// callee-saved preference either.
