// Decompiled by deepseek-v4.1-flash. Names are provisional.
// Sums the player's signed byte table (at +0x91) over the units within
// `range` of `pos`; the distance is the 64-bit high product (>> 32).

// Unused: <math.h>'s declarations keep the index and the table base in the
// registers the original lookup uses (docs/c2-regalloc.md).
#include <math.h>
#include <vector>

#include "../util/vec3.h"

// PlayerAI's members are laid out packed, so it stays inside the pragma.
#pragma pack(push, 1)
#include "player_ai.h"
#pragma pack(pop)

#include "../units/unit.h"

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
