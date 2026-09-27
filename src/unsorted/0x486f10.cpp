// Decompiled by space-bunny-free. Names are provisional.

void __stdcall FUN_0049b000(void* unit, int second);
void __stdcall FUN_00489bb0(void* a, void* b, int c, int d, int e);
void __stdcall FUN_004864b0(void* unit, int param_2);

#pragma pack(push, 1)
struct UnitDef_00486f10 {
    int active;                        // +0x00
    char unknown_4[0x73 - 0x4];
    unsigned char type;                // +0x73
};

struct Unit_00486f10 {
    char unknown_0[0x96];
    UnitDef_00486f10* def;             // +0x96
    char unknown_9a[0x110 - 0x9a];
    unsigned int flags;                // +0x110
    char unknown_114[0x118 - 0x114];
};

struct Player_00486f10 {
    char unknown_0[0x67];
    Unit_00486f10* first_unit;         // +0x67
    Unit_00486f10* last_unit;          // +0x6b
    char unknown_6f[0x144 - 0x6f];
    unsigned short field_144;          // +0x144
    char unknown_146[0x14b - 0x146];
};

struct Game_00486f10 {
    char unknown_0[0x1b63];
    Player_00486f10 players[10];       // +0x1b63
};
#pragma pack(pop)

extern Game_00486f10* g_game;

// Fires one of a unit's two weapons (0x49b000) for every unit of the player
// whose flags have bit 0x10000000 set and bit 0x4000 clear, then sets that
// bit and calls FUN_004864b0 with team 3. Units whose definition is of type 1
// or 2 go to FUN_00489bb0 instead. The unit list is read through a fresh
// players[] expression rather than through the tested pointer p: that keeps
// the element offset in eax and the tested pointer in ecx, as the original
// does. Compare 0x486ed0 for the same unit loop over g_game->units.
// FUNCTION: 0x486f10
void __stdcall FUN_00486f10(unsigned char player)
{
    Player_00486f10* p = &g_game->players[player];
    if (p != 0) {
        if (p->field_144 != 0) {
            Unit_00486f10* u = g_game->players[player].first_unit;
            Unit_00486f10* end = g_game->players[player].last_unit;
            if (u != 0) {
                for (; u <= end; u = (Unit_00486f10*)((char*)u + 0x118)) {
                    if (u->flags & 0x10000000) {
                        if (!(u->flags & 0x4000)) {
                            UnitDef_00486f10* def = u->def;
                            if (def->active == 0
                                || (def->type != 1 && def->type != 2)) {
                                FUN_0049b000(u, 1);
                                u->flags |= 0x4000;
                                FUN_004864b0(u, 3);
                            } else {
                                FUN_00489bb0(u, u, 30000, 3, 0);
                            }
                        }
                    }
                }
            }
        }
    }
}
