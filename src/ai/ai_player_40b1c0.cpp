// Decompiled by deepseek-v4.1-flash. Names are provisional.
// Sums the player's signed byte table (at +0x91) over the units within
// `range` of `pos`; the distance is the 64-bit high product (>> 32).

#include <vector>
#include "../util/vec3.h"
#pragma pack(push, 1)
struct Unit {
    char unknown_0[0x6a];
    Vec3 pos;                          // +0x6a
    char unknown_76[0xa6 - 0x76];
    unsigned short unitDefIndex;       // +0xa6
};

#include "player_ai.h"
#pragma pack(pop)

extern PlayerAI* g_playerAI[];

// FUNCTION: 0x40b1c0
int __stdcall SumUnitRatingsInRange(int player, Vec3* pos, int range)
{
    int total = 0;
    PlayerAI* p = g_playerAI[player];
    int r2 = range * range;
    std::vector<Unit*>& units = p->visible.units;
    for (std::vector<Unit*>::iterator it = units.begin(); it != units.end(); it++) {
        Unit* u = *it;
        int dz = pos->z - u->pos.z;
        int dx = pos->x - u->pos.x;
        int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
        if (d <= r2)
            total += (signed char)p->vec_8d[u->unitDefIndex];
    }
    return total;
}
