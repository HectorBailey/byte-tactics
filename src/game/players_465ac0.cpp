// Decompiled by space-bunny-free, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, ninth pass by space-bunny-free, finished by opus. Names are provisional.
//
// What the function does: a visibility test for one unit. It returns 1 at once
// What the function does: a visibility test for one unit. It returns 1 at once
// if the unit's player pointer (player) is the map asked about, or 0 if flag 4
// of the unit's activateFlags is set. Otherwise it builds a 16.16 position from
// the unit's position plus the def's offsets, bails out when the position is
// below the game-wide limit, and then asks "is this point visible" four times:
// at p, at p + (def->modelSizeX,0,0), at p + (modelSizeX,-modelSizeY,
// modelSizeZ), and finally at p again. The first three go through IsPointVisible unless the player's map
// flags say to use the explored byte map instead; the fourth inlines the shared
// visibility bit mask.
#include "../map/byte_map.h"
#pragma pack(push, 1)
// A player slot read for the explored grid at +0x7c (network/player.h's
// explored, exploredWidth and exploredHeight). The same record in every file
// that tests visibility against it; the unit's player pointer is cast to it,
// since Player itself is not a view of just that grid.
struct ExploredMap {
    char unknown_0[0x7c];
    ByteMap explored;                    // +0x7c
};
struct Game {
    char unknown_0[0x2a43];
    unsigned char playerIndex;           // +0x2a43
    char unknown_2a44[0x14273 - 0x2a44];
    unsigned short* visibilityMask;      // +0x14273
    char unknown_14277[0x1427f - 0x14277];
    unsigned char seaLevel;              // +0x1427f
    char debugMode;
    unsigned short mapFlags;             // +0x14281
};
#include "../units/unit_def.h"
#include "../util/vec3.h"
#include "../units/unit.h"
// The 16.16 position the fog test builds and reads, the same Fixed and
// FixedVec3 graphics/particles_475470.cpp declares; only the whole words are
// read here.
struct Fixed {
    unsigned short frac;
    short whole;
};

struct FixedVec3 {
    Fixed x;
    Fixed y;
    Fixed z;
};
#pragma pack(pop)

extern Game* g_game;

int __stdcall IsPointVisible(ExploredMap* map, FixedVec3* pos);

// the player's explored byte map, inlined where the game map flags ask for it
static inline int IsExplored(ExploredMap* map, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty)) {
        if (map->explored.Get(tx, ty) != 0)
            return 1;
    }
    return 0;
}
static inline int IsSeen(ExploredMap* map, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (!map->explored.size.Contains(tx, ty))
        return 0;
    return (g_game->visibilityMask[map->explored.size.width * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

// Plain index here, Get method in IsExplored: one helper for tests 1 to 3
// does not match.
static inline int IsExplored2(ExploredMap* map, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty)) {
        if (map->explored.data[map->explored.size.width * ty + tx] != 0)
            return 1;
    }
    return 0;
}
// IsVisible and IsVisible2 stay single ternaries, not an if with two returns.
static inline int IsVisible(ExploredMap* map, FixedVec3* pos)
{
    return (g_game->mapFlags & 2) == 2 ? IsExplored(map, pos) : IsPointVisible(map, pos);
}
static inline int IsVisible2(ExploredMap* map, FixedVec3* pos)
{
    return (g_game->mapFlags & 2) == 2 ? IsExplored2(map, pos) : IsPointVisible(map, pos);
}
// Branchy shape stays: lets nothing range-track the value.
static inline int IsExplored3(ExploredMap* map, FixedVec3* pos)
{
    int tx = pos->x.whole >> 5;
    int ty = (pos->z.whole - (pos->y.whole >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty)) {
        unsigned char* d = map->explored.data + map->explored.size.width * ty;
        if (d[tx] != 0)
            return 1;
    }
    return 0;
}


// Stays an if: gives the normalisation on all four exits.
static inline int IsVisible3(ExploredMap* map, FixedVec3* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored3(map, pos);
    return IsSeen(map, pos);
}
// FUNCTION: 0x465ac0
int __stdcall IsUnitVisibleToPlayer(ExploredMap* map, Unit* u)
{
    if ((ExploredMap*)u->player == map)
        return 1;
    if (u->cloaked)
        return 0;
    Vec3 p;
    p.x = u->def->modelMinX + u->pos.x;
    p.y = u->def->modelMaxY + u->pos.y;
    p.z = u->def->modelMinZ + u->pos.z;
    if (!(u->flags & 0x200)) {
        if (p.y < (g_game->seaLevel << 16))
            return 0;
    }
    if (IsVisible(map, (FixedVec3*)&p))
        return 1;
    p.x += u->def->modelSizeX;
    if (IsVisible2(map, (FixedVec3*)&p))
        return 1;
    p.z += u->def->modelSizeZ;
    p.y -= u->def->modelSizeY;
    if (IsVisible2(map, (FixedVec3*)&p))
        return 1;
    p.x -= u->def->modelSizeX;
    if (IsVisible3(map, (FixedVec3*)&p))
        return 1;
    return 0;
}
