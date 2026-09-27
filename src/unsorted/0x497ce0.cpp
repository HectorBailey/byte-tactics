// Decompiled by space-bunny-free. Names are provisional.
// Draws the "waiting for other players" screen: if bit 3 of the network flags
// is set it prints "Synchronization complete", otherwise it draws one progress
// bar per player record, each 620/nPlayers wide with a fill proportional to
// that record's percent byte, and then the "Waiting for other players. N
// players ready" line.
//
// 79% (595-byte original). What matches: the prologue, the three init calls,
// the flags test, both loops' shapes, the whole first loop including its
// register assignment (countA in memory, countB in ebx, the loaded[] walk in
// esi, the counter in ebp), the division, and the two sprintf calls.
//
// Two spellings were needed to get there and are load bearing:
//   - the two player filters in loop 1 are two separate `if` statements, not a
//     nested if. Only that shape leaves the first count in memory and gives
//     the first loop the original's register assignment.
//   - the flags test needs a byte temporary (`sync = netFlags >> 3`) and the
//     field must be `volatile`, or MSVC folds the shift into `test byte,8`.
//
// What still differs, all register or scheduling choices inside loop 2 and the
// sprintf tail:
//   - the loop 2 temporaries are rotated: ours allocates eax/ecx/edx where the
//     original allocates ecx/edx/eax for the x2 value, the bar colour and &r,
//     and the same rotation reappears in the /100 sign fixup (ours `mov ecx,
//     edx; shr ecx,31`, the original `mov eax,edx; xor ecx,ecx; shr eax,31`).
//   - the preheader emits `mov esi,0xb` before the loop index store; the
//     original has the store first. No source spelling tried (for-init order,
//     separate declarations, while loop, unsigned, placement of the rect init)
//     changes it.
//   - the sprintf tail keeps the "player ready" literal in eax instead of
//     pushing it in each arm of the ternary.
#include <stdio.h>

#pragma pack(push, 1)

struct Rect_00497ce0 {
    int x1;
    int y1;
    int x2;
    int y2;
};

struct PlayerRec_00497ce0 {            // 0x14b bytes, array at g_game+0x1b63
    int present;                       // +0x00
    char unknown_4[0x20 - 0x4];
    unsigned char percent;             // +0x20
    char unknown_21[0x2b - 0x21];
    char name[0x73 - 0x2b];            // the text drawn beside the bar
    unsigned char team;                // +0x73, 1, 2 or 3
    char unknown_74[0x146 - 0x74];
    unsigned char kind;                // +0x146, 10 means not counted
    char unknown_147[0x14b - 0x147];
};

struct Menu_00497ce0 {
    char unknown_0[0x18];
    void* data;                        // +0x18
};

struct Game_00497ce0 {
    char unknown_0[0x519];
    Menu_00497ce0 menu;                // +0x519
    char unknown_519[0xdcf - 0x519 - sizeof(Menu_00497ce0)];
    unsigned char color1;              // +0xdcf, empty bar
    char unknown_dd0[0xdd5 - 0xdd0];
    unsigned char color2;              // +0xdd5, filled bar
    char unknown_dd6[0x1b63 - 0xdd6];
    PlayerRec_00497ce0 players[10];    // +0x1b63
    char unknown_2851[0x29a4 - 0x2851];
    int loaded[10];                    // +0x29a4
    char unknown_29cc[0x38d75 - 0x29cc];
    volatile unsigned char netFlags;   // +0x38d75, volatile in the original (see 0x494e70.cpp)
};
#pragma pack(pop)

extern Game_00497ce0* g_game;

void __stdcall FUN_004a81e0(Menu_00497ce0* menu, int value);
void __stdcall FUN_0049fad0(Menu_00497ce0* menu);
void __stdcall FUN_004ab170(Menu_00497ce0* menu, int a, int b);
void __stdcall FUN_004bf6f0(void* surface, Rect_00497ce0* rect, int color);
void __stdcall FUN_004a50e0(void* surface, const char* text, int x, int y, int len, int flag);
char* __stdcall FUN_004c5740(char* s);

// FUNCTION: 0x497ce0
void __stdcall FUN_00497ce0(void* surface)
{
    FUN_004a81e0(&g_game->menu, 0x40);
    FUN_0049fad0(&g_game->menu);
    FUN_004ab170(&g_game->menu, 0, 0);

    const char* text;
    unsigned char sync = g_game->netFlags >> 3;
    if ((sync & 1) != 0) {
        // the return value stays in eax all the way to the draw call
        text = FUN_004c5740("Synchronization complete");
    } else {
        int numReady = 0;
        int numLoaded = 0;
        for (int i = 0; i < 10; i++) {
            PlayerRec_00497ce0* p = &g_game->players[i];
            if (p->present && (p->team == 1 || p->team == 2 || p->team == 3) && p->kind != 10)
                numReady++;
            if (p->present && (p->team == 1 || p->team == 2 || p->team == 3) && p->kind != 10 &&
                p->percent == 100 && g_game->loaded[i] != 0)
                numLoaded++;
        }

        // no guard: with no player record passing the filter this divides by zero
        int slot = 620 / numReady;
        Rect_00497ce0 r;
        r.x1 = 10;
        r.y1 = 420;
        r.y2 = 435;
        for (int j = 0, x = 11; j < 10; j++) {
            PlayerRec_00497ce0* q = &g_game->players[j];
            if (q->present && (q->team == 1 || q->team == 2 || q->team == 3) && q->kind != 10) {
                r.x1 = x;
                r.x2 = slot + x - 2;
                FUN_004bf6f0(surface, &r, g_game->color1);
                int pc = q->percent;
                r.x2 = r.x1 + (pc * (slot - 2)) / 100;
                FUN_004bf6f0(surface, &r, g_game->color2);
                FUN_004a50e0(surface, q->name, r.x1, 420, slot - 2, 0);
                x += slot;
            }
        }

        char buf[128];
        sprintf(buf, "%s.  %i %s", FUN_004c5740("Waiting for other players"), numLoaded,
            FUN_004c5740(numLoaded == 1 ? "player ready" : "players ready"));
        text = buf;
    }

    FUN_004a50e0(surface, text, 10, 400, -1, 0);
}
