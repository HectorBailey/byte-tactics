// Decompiled by space-bunny-free. Names are provisional.
// Shuts the unit table down: kills every live unit in the block at +0x14357
// (loop bounded by the end pointer at +0x1435b), then releases the two
// auxiliary lists and the unit block itself, nulling each field after the
// free. The three frees are written out separately because the original
// re-reads g_game before every one of them, so it cannot have used a
// local pointer for them.

void __stdcall FUN_004864b0(void* unit, int param_2);
void FUN_004d85a0(void* p);

#pragma pack(push, 1)
struct Unit_00485980 {
    char unknown_0[0xa6];
    short field_a6;                     // +0xa6
    char unknown_a8[0x118 - 0xa8];
};

struct Game_00485980 {
    char unknown_0[0x14357];
    Unit_00485980* units;               // +0x14357
    Unit_00485980* units_end;           // +0x1435b
    short* id_list;                     // +0x1435f
    short* other_list;                  // +0x14363
    int other_count;                    // +0x14367
};
#pragma pack(pop)

extern Game_00485980* g_game;

// FUNCTION: 0x485980
void FUN_00485980(void)
{
    Unit_00485980* u = g_game->units;
    Unit_00485980* end = g_game->units_end;
    if (u != 0) {
        for (; u <= end; u = (Unit_00485980*)((char*)u + 0x118)) {
            if (u->field_a6 != 0) {
                FUN_004864b0(u, 8);
            }
        }
    }
    if (g_game->other_list != 0) {
        FUN_004d85a0(g_game->other_list);
    }
    g_game->other_list = 0;
    if (g_game->id_list != 0) {
        FUN_004d85a0(g_game->id_list);
    }
    g_game->id_list = 0;
    if (g_game->units != 0) {
        FUN_004d85a0(g_game->units);
    }
    g_game->units = 0;
}
