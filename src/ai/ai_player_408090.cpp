// Decompiled by Opus. Names are provisional.
// The first argument is the caller's player record, the same one
// ai_player_407e70.cpp reads: its explored map starts at +0x7c, so the size the
// test below indexes the visibility mask by is at +0x80. Its own Game view
// stays local, since game/game.h's declarations move this function
// (docs/c2-regalloc.md).

#include "../map/byte_map.h"

#pragma pack(push, 1)
struct Game {
    char unknown_0[0x2a43];
    unsigned char playerIndex;          // +0x2a43
    char unknown_2a44[0x14273 - 0x2a44];
    unsigned short* visibilityMask;     // +0x14273
};

struct Player {
    char unknown_0[0x7c];
    ByteMap explored;                   // +0x7c
};
#pragma pack(pop)

// The 16.16 position the fog test reads; only the whole words are read here.
#include "../util/fixed.h"

extern Game* g_game;

// FUNCTION: 0x408090
int __stdcall IsPointVisible(Player* player, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (!player->explored.size.Contains(tx, ty)) {
        return 0;
    }
    return (g_game->visibilityMask[player->explored.size.width * ty + tx] & (1 << g_game->playerIndex)) != 0;
}
