// Decompiled by GPT-6. Names are provisional.
struct Unit;
void __stdcall CopyDwordIfNonNull(Unit**, Unit* const*);
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull(dest, &src); }
}
#include <vector>
#include "../units/unit.h"
struct Vec { int x,y,z; };
#pragma pack(push,1)
struct Owner { char pad[0x25]; std::vector<Unit*> factories; };
#pragma pack(pop)
extern Owner* g_playerAI[];
// FUNCTION: 0x40b530
void __stdcall GetFactoriesInRadius(int player,const Vec* pos,int radius,std::vector<Unit*>* out)
{
    std::vector<Unit*>& list=g_playerAI[player]->factories;
    radius*=radius;
    for(std::vector<Unit*>::iterator it=list.begin();it!=list.end();++it) {
        Unit* unit=*it;
        if((unit->def->flags1&0x40) && (unit->def->flags1&0x200) && (unit->on)) {
            int dz=pos->z-unit->pos.z;
            int dx=pos->x-unit->pos.x;
            int d=(int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32);
            if(d<=radius) out->push_back(*it);
        }
    }
}
